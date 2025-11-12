package main

import (
	"errors"
	"fmt"
	"sort"
	"sync"
	"sync/atomic"
	"time"
)

// ---------- Domain types ----------

type AccountID string
type TxnID uint64

// Posting: a single debit/credit in a double-entry ledger.
// Convention: credit > 0, debit < 0. Sum(postings.Amount) must be 0.
type Posting struct {
	Account AccountID
	Amount  int64 // +credit to account, -debit from account
}

type LedgerEntry struct {
	ID             TxnID
	At             time.Time
	Description    string
	Postings       []Posting
	IdempotencyKey string
}

// ---------- Errors ----------

var (
	ErrBadAmount          = errors.New("amount must be > 0")
	ErrInsufficientFunds  = errors.New("insufficient funds")
	ErrAccountExists      = errors.New("account already exists")
	ErrAccountNotFound    = errors.New("account not found")
	ErrIdempotentConflict = errors.New("idempotency key already used for different op")
)

// ---------- Wallet (in-memory) ----------

type Wallet struct {
	mu        sync.RWMutex
	balances  map[AccountID]int64    // account -> current balance
	entries   map[TxnID]*LedgerEntry // txn id -> ledger entry
	byAccount map[AccountID][]TxnID  // index: account -> ordered txn ids
	idem      map[string]TxnID       // idempotency key -> txn id
	nextID    atomic.Uint64          // txn id generator
	system    AccountID              // system equity account for mint/burn
}

// ---------- Constructor ----------

func NewWallet() *Wallet {
	w := &Wallet{
		balances:  make(map[AccountID]int64),
		entries:   make(map[TxnID]*LedgerEntry),
		byAccount: make(map[AccountID][]TxnID),
		idem:      make(map[string]TxnID),
		system:    AccountID("=SYSTEM="),
	}
	// Register the system equity account.
	w.balances[w.system] = 0
	return w
}

// ---------- Helpers ----------

func (w *Wallet) hasAccount(id AccountID) bool {
	_, ok := w.balances[id]
	return ok
}

func abs(x int64) int64 {
	if x < 0 {
		return -x
	}
	return x
}

// ---------- API ----------

// CreateAccount registers a new account with zero balance.
func (w *Wallet) CreateAccount(id AccountID) error {
	w.mu.Lock()
	defer w.mu.Unlock()
	if w.hasAccount(id) {
		return ErrAccountExists
	}
	w.balances[id] = 0
	return nil
}

// GetBalance returns the current balance for an account.
func (w *Wallet) GetBalance(id AccountID) (int64, error) {
	w.mu.RLock()
	defer w.mu.RUnlock()
	if !w.hasAccount(id) {
		return 0, ErrAccountNotFound
	}
	return w.balances[id], nil
}

// ListTransactions returns transactions involving the account, newest first.
func (w *Wallet) ListTransactions(id AccountID) ([]LedgerEntry, error) {
	w.mu.RLock()
	defer w.mu.RUnlock()
	if !w.hasAccount(id) {
		return nil, ErrAccountNotFound
	}
	ids := w.byAccount[id]
	out := make([]LedgerEntry, 0, len(ids))
	for _, tid := range ids {
		if e, ok := w.entries[tid]; ok {
			out = append(out, *e)
		}
	}
	sort.SliceStable(out, func(i, j int) bool { return out[i].At.After(out[j].At) })
	return out, nil
}

// AddMoney credits 'to' from the system account (mints from system equity).
// Idempotent when idemKey is reused for the same call semantics.
func (w *Wallet) AddMoney(to AccountID, amount int64, description, idemKey string) (TxnID, error) {
	if amount <= 0 {
		return 0, ErrBadAmount
	}
	postings := []Posting{
		{Account: w.system, Amount: -amount}, // system pays
		{Account: to, Amount: +amount},       // user receives
	}
	return w.apply(postings, description, idemKey, func() error {
		if !w.hasAccount(to) {
			return ErrAccountNotFound
		}
		// system is always present in balances
		return nil
	})
}

// Transfer moves funds between two user accounts (debit from, credit to).
// Idempotent via idemKey.
func (w *Wallet) Transfer(from, to AccountID, amount int64, description, idemKey string) (TxnID, error) {
	if amount <= 0 {
		return 0, ErrBadAmount
	}
	if from == to {
		return 0, errors.New("cannot transfer to same account")
	}
	postings := []Posting{
		{Account: from, Amount: -amount},
		{Account: to, Amount: +amount},
	}
	return w.apply(postings, description, idemKey, func() error {
		if !w.hasAccount(from) || !w.hasAccount(to) {
			return ErrAccountNotFound
		}
		if w.balances[from] < amount {
			return ErrInsufficientFunds
		}
		return nil
	})
}

// apply posts a double-entry transaction with idempotency & invariants.
func (w *Wallet) apply(postings []Posting, description, idemKey string, precheck func() error) (TxnID, error) {
	now := time.Now()

	// Invariant: sum amounts == 0
	var sum int64
	for _, p := range postings {
		sum += p.Amount
	}
	if sum != 0 {
		return 0, errors.New("unbalanced postings (sum != 0)")
	}

	w.mu.Lock()
	defer w.mu.Unlock()

	// Idempotency: if key already used, return prior txn id.
	if idemKey != "" {
		if prior, ok := w.idem[idemKey]; ok {
			// Optional: validate same semantic request (hash postings) before accepting reuse.
			return prior, nil
		}
	}

	// Pre-checks under lock to avoid TOCTOU.
	if err := precheck(); err != nil {
		return 0, err
	}

	// Apply postings atomically.
	for _, p := range postings {
		if !w.hasAccount(p.Account) {
			return 0, ErrAccountNotFound
		}
		w.balances[p.Account] += p.Amount
	}

	id := TxnID(w.nextID.Add(1))
	entry := &LedgerEntry{
		ID:             id,
		At:             now,
		Description:    description,
		Postings:       append([]Posting(nil), postings...),
		IdempotencyKey: idemKey,
	}
	w.entries[id] = entry

	// Update per-account index once per unique account in this txn.
	seen := make(map[AccountID]struct{}, len(postings))
	for _, p := range postings {
		if _, done := seen[p.Account]; done {
			continue
		}
		seen[p.Account] = struct{}{}
		w.byAccount[p.Account] = append(w.byAccount[p.Account], id)
	}

	// Record idempotency key.
	if idemKey != "" {
		w.idem[idemKey] = id
	}

	return id, nil
}

// ---------- Printing helpers ----------

// PrintAllTransactions prints the entire ledger (newest first).
func (w *Wallet) PrintAllTransactions() {
	w.mu.RLock()
	defer w.mu.RUnlock()

	if len(w.entries) == 0 {
		fmt.Println("No transactions yet.")
		return
	}

	// Collect and sort all entries by time (newest first)
	all := make([]*LedgerEntry, 0, len(w.entries))
	for _, e := range w.entries {
		all = append(all, e)
	}
	sort.SliceStable(all, func(i, j int) bool { return all[i].At.After(all[j].At) })

	fmt.Println("\n========= GLOBAL LEDGER =========")
	for _, e := range all {
		fmt.Printf("TxnID: %d | %s | %s\n", e.ID, e.At.Format(time.RFC3339), e.Description)
		for _, p := range e.Postings {
			sign := " "
			if p.Amount > 0 {
				sign = "+"
			}
			fmt.Printf("   %-8s %s%d\n", p.Account, sign, p.Amount)
		}
		fmt.Println("---------------------------------")
	}
	fmt.Println("=================================\n")
}

// PrintUserTransactions prints all transactions for a given account.
func (w *Wallet) PrintUserTransactions(id AccountID) {
	w.mu.RLock()
	defer w.mu.RUnlock()

	if !w.hasAccount(id) {
		fmt.Printf("❌ Account '%s' not found.\n", id)
		return
	}

	ids := w.byAccount[id]
	if len(ids) == 0 {
		fmt.Printf("ℹ️  No transactions found for account '%s'.\n", id)
		return
	}

	// Gather entries in original order (we recorded by creation time).
	entries := make([]*LedgerEntry, 0, len(ids))
	for _, tid := range ids {
		if e, ok := w.entries[tid]; ok {
			entries = append(entries, e)
		}
	}
	// Show newest first for convenience
	sort.SliceStable(entries, func(i, j int) bool { return entries[i].At.After(entries[j].At) })

	fmt.Printf("\n========= TRANSACTIONS for %s =========\n", id)
	for _, e := range entries {
		fmt.Printf("TxnID: %d | %s | %s\n", e.ID, e.At.Format(time.RFC3339), e.Description)

		// Print this account's posting and counter-postings
		for _, p := range e.Postings {
			if p.Account == id {
				sign := "+"
				if p.Amount < 0 {
					sign = "-"
				}
				fmt.Printf("   %-8s %s%d\n", id, sign, abs(p.Amount))
			}
		}
		// Counterparties (optional nice touch)
		for _, p := range e.Postings {
			if p.Account != id {
				role := "from"
				val := p.Amount
				// If THIS posting is positive, that account was credited; relative to id, show direction.
				// We'll just print the raw posting for counterparties for clarity.
				if val < 0 {
					role = "to"
				}
				fmt.Printf("   %s %-8s %+d\n", role, p.Account, p.Amount)
			}
		}
		fmt.Println("---------------------------------")
	}
	fmt.Println("=================================\n")
}

// ---------- Demo ----------

func main() {
	w := NewWallet()

	must("create alice", w.CreateAccount("alice"))
	must("create bob", w.CreateAccount("bob"))

	// Fund Alice from system (idempotent)
	t1, err := w.AddMoney("alice", 10_000, "Initial top-up ₹100", "idem-1")
	must("add alice", err)
	fmt.Println("Topup txn:", t1)

	// Re-using the same idempotency key returns the same txn id
	t1b, err := w.AddMoney("alice", 10_000, "Initial top-up duplicate", "idem-1")
	must("idem replay", err)
	fmt.Println("Topup idem replay txn:", t1b)

	t3, err := w.AddMoney("alice", 10_000, "Top-up of ₹100", "idem-3")
	must("add alice", err)
	fmt.Println("Topup txn:", t3)

	// Transfer Alice -> Bob (₹30)
	t2, err := w.Transfer("alice", "bob", 3_000, "Lunch split", "idem-2")
	must("transfer", err)
	fmt.Println("Transfer txn:", t2)

	t4, err := w.Transfer("bob", "alice", 1_000, "Taxi fare", "idem-4")
	must("transfer", err)
	fmt.Println("Transfer txn:", t4)

	// Balances
	ba, _ := w.GetBalance("alice")
	bb, _ := w.GetBalance("bob")
	fmt.Printf("Alice balance: %d paise\n", ba)
	fmt.Printf("Bob   balance: %d paise\n", bb)

	// Print per-user transactions
	w.PrintUserTransactions("alice")
	w.PrintUserTransactions("bob")

	// Print the whole ledger
	w.PrintAllTransactions()
}

func must(msg string, err error) {
	if err != nil {
		panic(fmt.Sprintf("%s: %v", msg, err))
	}
}

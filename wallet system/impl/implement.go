package main

import (
	"errors"
	"fmt"
	"sort"
	"sync"
	"sync/atomic"
	"time"
)

type AccountID string
type TxnID uint64

type Posting struct {
	Account AccountID
	Amount  int64
}

type LedgerEntry struct {
	ID             TxnID
	At             time.Time
	Description    string
	Postings       []Posting
	IdempotencyKey string
}

var (
	ErrAccountExists   = errors.New("Acount already exists")
	ErrAccountNotFound = errors.New("Account not found")
	ErrBadAmount       = errors.New("Amount less than 0")
)

type Wallet struct {
	mu        sync.RWMutex
	balance   map[AccountID]int64
	entries   map[TxnID]*LedgerEntry
	byAccount map[AccountID][]TxnID
	idem      map[string]TxnID
	nextID    atomic.Uint64
	system    AccountID
}

func NewWallet() *Wallet {
	w := &Wallet{
		balance:   make(map[AccountID]int64),
		entries:   make(map[TxnID]*LedgerEntry),
		byAccount: make(map[AccountID][]TxnID),
		idem:      make(map[string]TxnID),
		system:    AccountID("=SYSTEM="),
	}

	w.balance[w.system] = 0
	return w
}

func (w *Wallet) hasAccount(id AccountID) bool {
	_, ok := w.balance[id]
	return ok
}

func abs(x int64) int64 {
	if x < 0 {
		return -x
	}
	return x
}

func (w *Wallet) CreateAccount(id AccountID) error {
	w.mu.Lock()
	defer w.mu.Unlock()
	if w.hasAccount(id) {
		return ErrAccountExists
	}
	w.balance[id] = 0
	return nil
}

func (w *Wallet) GetBalance(id AccountID) (int64, error) {
	w.mu.RLock()
	defer w.mu.RUnlock()
	if !w.hasAccount(id) {
		return 0, ErrAccountNotFound
	}
	return w.balance[id], nil
}

func (w *Wallet) AddMoney(to AccountID, amount int64, description, idemKey string) (TxnID, error) {
	if amount <= 0 {
		return 0, ErrBadAmount
	}
	postings := []Posting{
		{Account: w.system, Amount: -amount},
		{Account: to, Amount: +amount},
	}

	return w.apply(postings, description, idemKey, func() error {
		if !w.hasAccount(to) {
			return ErrAccountNotFound
		}
		return nil
	})
}

func (w *Wallet) Transfer(from, to AccountID, amount int64, description, idemKey string) (TxnID, error) {
	if amount <= 0 {
		return 0, ErrBadAmount
	}

	if from == to {
		return 0, errors.New("Cannnot do self transfer")
	}

	posting := []Posting{
		{Account: from, Amount: -amount},
		{Account: to, Amount: +amount},
	}

	return w.apply(posting, description, idemKey, func() error {
		if !w.hasAccount(from) || !w.hasAccount(to) {
			return ErrAccountNotFound
		}
		if w.balance[from] < amount {
			return errors.New("Insufficient Balance")
		}
		return nil
	})
}

func (w *Wallet) apply(postings []Posting, description, idemKey string, precheck func() error) (TxnID, error) {
	now := time.Now()

	var sum int64
	for _, p := range postings {
		sum += p.Amount
	}
	if sum != 0 {
		return 0, errors.New("unbalanced posting (sum != 0)")
	}

	w.mu.Lock()
	defer w.mu.Unlock()

	if idemKey != "" {
		if prior, ok := w.idem[idemKey]; ok {
			return prior, nil
		}
	}

	if err := precheck(); err != nil {
		return 0, err
	}

	for _, p := range postings {
		if !w.hasAccount(p.Account) {
			return 0, ErrAccountNotFound
		}
		w.balance[p.Account] += p.Amount
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

	seen := make(map[AccountID]struct{}, len(postings))
	for _, p := range postings {
		if _, done := seen[p.Account]; done {
			continue
		}
		seen[p.Account] = struct{}{}
		w.byAccount[p.Account] = append(w.byAccount[p.Account], id)
	}

	if idemKey != "" {
		w.idem[idemKey] = id
	}

	return id, nil
}

func (w *Wallet) PrintAllTransactions() {
	w.mu.RLock()
	defer w.mu.RUnlock()

	if len(w.entries) == 0 {
		fmt.Println("No transactions")
		return
	}

	all := make([]*LedgerEntry, 0, len(w.entries))
	for _, e := range w.entries {
		all = append(all, e)
	}
	sort.Slice(all, func(i, j int) bool { return all[i].At.Before(all[j].At) }) // oldest first

	for _, e := range all {
		fmt.Printf("TXN %d %s %q\n", e.ID, time.Now(), e.Description)
		for _, p := range e.Postings {
			fmt.Printf("  %s %d\n", p.Account, p.Amount)
		}
	}
}

func (w *Wallet) PrintTransactionsUser(id AccountID) {
	w.mu.RLock()
	defer w.mu.RUnlock()

	if _, ok := w.balance[id]; !ok {
		fmt.Printf("Account %q not found\n", id)
		return
	}

	ids := w.byAccount[id]
	if len(ids) == 0 {
		fmt.Printf("No transactions for %s\n", id)
		return
	}

	for tid := range ids {
		e, ok := w.entries[TxnID(tid)]
		if !ok {
			continue
		}
		fmt.Printf("TXN %d %s %q\n", e.ID, time.Now(), e.Description)
		for _, p := range e.Postings {
			if p.Account == id {
				fmt.Printf("  %s %d\n", id, p.Amount)
			}
		}
	}

}

func main() {
	w := NewWallet()

	must("create Utkarsh", w.CreateAccount("Utkarsh"))
	must("create Harsh", w.CreateAccount("Harsh"))

	t1, err := w.AddMoney("Utkarsh", 10_000, "Top-up from the system", "idem-1")
	must("money add in Utkarsh", err)
	fmt.Println("Topup Utkarsh:", t1)

	t2, err := w.AddMoney("Harsh", 5_000, "Top-up from the system", "idem-2")
	must("money added in Harsh", err)
	fmt.Println("Topup Harsh", t2)

	b1, err := w.GetBalance("Utkarsh")
	must("Balance Utkarsh", err)
	fmt.Println("Balance of Utkarsh", b1)

	b2, err := w.GetBalance("Harsh")
	must("Balance Harsh", err)
	fmt.Println("Balance of Utkarsh", b2)

	t3, err := w.Transfer("Utkarsh", "Harsh", 200, "Food today", "idem-3")
	must("transfering Harsh", err)
	fmt.Println("Transfered amount", t3)

	w.PrintAllTransactions()

	b3, err := w.GetBalance("Utkarsh")
	must("Balance Utkarsh", err)
	fmt.Println("Balance of Utkarsh", b3)

	b4, err := w.GetBalance("Harsh")
	must("Balance Harsh", err)
	fmt.Println("Balance of Utkarsh", b4)

}

func must(msg string, err error) {
	if err != nil {
		panic(fmt.Sprintf("%s: %v", msg, err))
	}
}

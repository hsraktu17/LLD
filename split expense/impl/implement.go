package main

import (
	"fmt"
	"math"
)

type User struct {
	Id   string
	Name string
}

type ExpenseManager struct {
	users    map[string]*User
	balances map[string]map[string]float64
}

func NewExpenseManager() *ExpenseManager {
	return &ExpenseManager{
		users:    make(map[string]*User),
		balances: make(map[string]map[string]float64),
	}
}

func (em *ExpenseManager) ensureUser(id string) {
	if _, ok := em.users[id]; !ok {
		em.users[id] = &User{Id: id, Name: id}
	}
	if _, ok := em.balances[id]; !ok {
		em.balances[id] = make(map[string]float64)
	}
}

func round2(x float64) float64 {
	return math.Round(x*100) / 100
}

func feq(a, b float64) bool {
	const eps = 1e-6
	return math.Abs(a-b) <= eps
}

func (em *ExpenseManager) AddUser(id, name string) {
	em.ensureUser(id)
	em.users[id].Name = name
}

func (em *ExpenseManager) addExpense(payer string, participants []string, shares []float64) {
	for i := 0; i < len(participants); i++ {
		user := participants[i]
		if user == payer {
			continue
		}
		share := shares[i]

		em.ensureUser(user)
		em.ensureUser(payer)

		em.balances[user][payer] = round2(em.balances[user][payer] + share)
		em.balances[payer][user] = round2(em.balances[payer][user] - share)
	}
}

func (em *ExpenseManager) ProcessExpense(payer string, amount float64, numUsers int, participants []string, typ string, shares []float64) {
	em.ensureUser(payer)

	for _, u := range participants {
		em.ensureUser(u)
	}

	amounts := make([]float64, numUsers)

	switch typ {
	case "EQUAL":
		if numUsers == 0 {
			fmt.Println("Error: no participants")
			return
		}

		each := round2(amount / float64(numUsers))
		for i := range numUsers {
			amounts[i] = each
		}

		var sum float64

		for i := 0; i < numUsers-1; i++ {
			sum += amounts[i]
		}

		amounts[numUsers-1] = round2(amount - sum)

	case "EXACT":
		if len(shares) != numUsers {
			fmt.Println("Error: exact expects shares for all participants")
			return
		}

		var sum float64
		for _, s := range shares {
			sum += s
		}

		if !feq(sum, amount) {
			fmt.Println("Error: Exact amount do not sum up to total expense!")
		}
		copy(amounts, shares)

	case "PERCENT":
		if len(shares) != numUsers {
			fmt.Println("Error: PERCENT expects shares (percents) for all participants")
			return
		}
		var sum float64
		for _, s := range shares {
			sum += s
		}

		if !feq(sum, 100.0) {
			fmt.Println("Error:Percentage do not sum to 100!")
			return
		}

		for i := 0; i < numUsers; i++ {
			amounts[i] = round2((shares[i] / 100.0) * amount)
		}

		var s2 float64

		for i := 0; i < numUsers-1; i++ {
			s2 += amounts[i]
		}
		amounts[numUsers-1] = round2(amount - s2)
	default:
		fmt.Println("Error: unknown type:", typ)
		return
	}

	em.addExpense(payer, participants, amounts)
}

func (em *ExpenseManager) ShowBalance() {
	found := false
	for u, row := range em.balances {
		for v, val := range row {
			if val > 0 {
				fmt.Printf("%s owes %s: %.2f\n", u, v, val)
				found = true
			}
		}
	}
	if !found {
		fmt.Println("No Balances")
	}
}

func (em *ExpenseManager) ShowBalanceUser(id string) {
	em.ensureUser(id)
	found := false

	for other, val := range em.balances[id] {
		if val > 0 {
			fmt.Printf("%s owes %s: %.2f\n", id, other, val)
			found = true
		}
	}

	if !found {
		fmt.Println("No Balances")
	}
}

func main() {

	m := NewExpenseManager()

	m.AddUser("u1", "User1")
	m.AddUser("u2", "User2")
	m.AddUser("u3", "User3")
	m.AddUser("u4", "User4")

	m.ShowBalance()
	m.ShowBalanceUser("u1")

	m.ProcessExpense("u1", 1000, 4, []string{"u1", "u2", "u3", "u4"}, "EXACT", []float64{100, 400, 300, 200})
	m.ShowBalance()
	m.ShowBalanceUser("u1")

}

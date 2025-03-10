#include<bits/stdc++.h>
using namespace std;

class User {
public:
    string userId, name;
    User(string id, string n) : userId(id), name(n) {}
};

class ExpenseManager {
private:
    unordered_map<string, User*> users;
    unordered_map<string, unordered_map<string, double>> balances;
    
    void addExpense(string payer, double amount, vector<string> participants, vector<double> shares) {
        for (size_t i = 0; i < participants.size(); i++) {
            if (participants[i] != payer) {
                balances[participants[i]][payer] += shares[i];
                balances[payer][participants[i]] -= shares[i];
            }
        }
    }

public:
    void addUser(string id, string name) {
        users[id] = new User(id, name);
    }
    
    void processExpense(string payer, double amount, int numUsers, vector<string> participants, string type, vector<double> shares = {}) {
        vector<double> amounts(numUsers, 0);
        
        if (type == "EQUAL") {
            double equalShare = amount / numUsers;
            for (int i = 0; i < numUsers; i++) amounts[i] = equalShare;
        } 
        else if (type == "EXACT") {
            double sum = 0;
            for (double s : shares) sum += s;
            if (sum != amount) {
                cout << "Error: Exact amounts do not sum up to total expense!" << endl;
                return;
            }
            amounts = shares;
        } 
        else if (type == "PERCENT") {
            double sum = 0;
            for (double s : shares) sum += s;
            if (sum != 100) {
                cout << "Error: Percentage shares do not sum up to 100!" << endl;
                return;
            }
            for (size_t i = 0; i < shares.size(); i++) amounts[i] = (shares[i] / 100) * amount;
        }
        
        addExpense(payer, amount, participants, amounts);
    }
    
    void showBalances() {
        bool found = false;
        for (auto &user : balances) {
            for (auto &debt : user.second) {
                if (debt.second > 0) {
                    cout << user.first << " owes " << debt.first << ": " << fixed << setprecision(2) << debt.second << endl;
                    found = true;
                }
            }
        }
        if (!found) cout << "No balances" << endl;
    }
    
    void showUserBalance(string userId) {
        bool found = false;
        for (auto &debt : balances[userId]) {
            if (debt.second > 0) {
                cout << userId << " owes " << debt.first << ": " << fixed << setprecision(2) << debt.second << endl;
                found = true;
            }
        }
        for (auto &creditor : balances) {
            if (creditor.second[userId] > 0) {
                cout << creditor.first << " owes " << userId << ": " << fixed << setprecision(2) << creditor.second[userId] << endl;
                found = true;
            }
        }
        if (!found) cout << "No balances" << endl;
    }
};

int main() {
    ExpenseManager manager;
    
    // Hardcoded users
    manager.addUser("u1", "User1");
    manager.addUser("u2", "User2");
    manager.addUser("u3", "User3");
    manager.addUser("u4", "User4");
    
    // Hardcoded expenses
    cout<<"----------------------"<<endl;
    manager.showBalances();
    manager.showUserBalance("u1");
    cout<<"----------------------"<<endl;
    
    manager.processExpense("u1", 1000, 4, {"u1", "u2", "u3", "u4"}, "EQUAL");
    manager.showBalances();
    manager.showUserBalance("u1");
    cout<<"----------------------"<<endl;
    
    manager.processExpense("u1", 1250, 2, {"u2", "u3"}, "EXACT", {370, 880});
    manager.showBalances();
    cout<<"----------------------"<<endl;

    manager.processExpense("u4", 1200, 4, {"u1", "u2", "u3", "u4"}, "PERCENT", {40, 20, 20, 20});
    cout<<"----------------------"<<endl;
    manager.showUserBalance("u1");
    cout<<"----------------------"<<endl;
    manager.showUserBalance("u2");
    cout<<"----------------------"<<endl;
    manager.showUserBalance("u3");
    cout<<"----------------------"<<endl;
    manager.showUserBalance("u4");
    cout<<"----------------------"<<endl;
    manager.showBalances();
    
    return 0;
}

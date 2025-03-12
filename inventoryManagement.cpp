#include <iostream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

using namespace std;
using namespace std::chrono;

struct Product {
    string name;
    int inventoryCount;
};

struct Order {
    vector<string> productIds;
    vector<int> quantities;
    time_point<steady_clock> orderTime;
    bool confirmed;
};

class InventoryManager {
private:
    unordered_map<string, Product> inventory;
    unordered_map<string, Order> orders;
    unordered_map<string, int> blockedInventory;
    mutex mtx;

public:
    void createProduct(string productId, string name, int count) {
        lock_guard<mutex> lock(mtx);
        inventory[productId] = {name, count};
        cout << "Product created: " << productId << " -> (" << name << ", " << count << ")" << endl;
    }

    int getInventory(string productId) {
        lock_guard<mutex> lock(mtx);
        if (inventory.find(productId) != inventory.end()) {
            return inventory[productId].inventoryCount;
        }
        return -1;
    }

    void createOrder(vector<string> productIds, vector<int> quantityOrdered, string orderId) {
        lock_guard<mutex> lock(mtx);
        bool canBlock = true;

        for (size_t i = 0; i < productIds.size(); i++) {
            if (inventory[productIds[i]].inventoryCount < quantityOrdered[i]) {
                canBlock = false;
                break;
            }
        }

        if (canBlock) {
            for (size_t i = 0; i < productIds.size(); i++) {
                inventory[productIds[i]].inventoryCount -= quantityOrdered[i];
                blockedInventory[productIds[i]] += quantityOrdered[i];
            }
            orders[orderId] = {productIds, quantityOrdered, steady_clock::now(), false};
            cout << "Order " << orderId << " created and inventory blocked." << endl;

            thread(&InventoryManager::releaseBlockedInventory, this, orderId).detach();
        } else {
            cout << "Insufficient inventory to create order " << orderId << "." << endl;
        }
    }

    void confirmOrder(string orderId) {
        lock_guard<mutex> lock(mtx);
        if (orders.find(orderId) != orders.end() && !orders[orderId].confirmed) {
            for (size_t i = 0; i < orders[orderId].productIds.size(); i++) {
                blockedInventory[orders[orderId].productIds[i]] -= orders[orderId].quantities[i];
            }
            orders[orderId].confirmed = true;
            cout << "Order " << orderId << " confirmed and inventory permanently reduced." << endl;
        } else {
            cout << "Order " << orderId << " not found or already confirmed." << endl;
        }
    }

    void releaseBlockedInventory(string orderId) {
        this_thread::sleep_for(minutes(5));
        lock_guard<mutex> lock(mtx);
        if (orders.find(orderId) != orders.end() && !orders[orderId].confirmed) {
            for (size_t i = 0; i < orders[orderId].productIds.size(); i++) {
                inventory[orders[orderId].productIds[i]].inventoryCount += orders[orderId].quantities[i];
                blockedInventory[orders[orderId].productIds[i]] -= orders[orderId].quantities[i];
            }
            orders.erase(orderId);
            cout << "Order " << orderId << " was not confirmed in time. Inventory released back." << endl;
        }
    }
};

int main() {
    InventoryManager manager;

    manager.createProduct("1", "P1", 2);
    manager.createProduct("2", "P2", 5);
    manager.createProduct("3", "P3", 4);

    cout << "Inventory of P1: " << manager.getInventory("1") << endl;
    cout << "Inventory of P2: " << manager.getInventory("2") << endl;
    cout << "Inventory of P3: " << manager.getInventory("3") << endl;

    manager.createOrder({"1", "3"}, {1, 2}, "1");

    cout << "Inventory of P1 after order: " << manager.getInventory("1") << endl;
    cout << "Inventory of P3 after order: " << manager.getInventory("3") << endl;

    manager.confirmOrder("1");

    cout << "Final Inventory of P1: " << manager.getInventory("1") << endl;
    cout << "Final Inventory of P3: " << manager.getInventory("3") << endl;

    return 0;
}

#include<bits/stdc++.h>
#include<math.h>

using namespace std;

struct Location{
    int x, y;
    double distance(const Location &order){
        return sqrt(pow(x - order.x,2) + pow(y - order.y,2));
    }
};

struct User{
    string name;
    int age;
    char gender;
    Location location;
};

struct Driver{
    string name;
    int age;
    char gender;
    Location location;
    string vehicle;
    string vehicleNumber;
    bool available;
    double earnings;
};

class CabBookingSystem{
private:
    unordered_map<string, User> users;
    unordered_map<string, Driver> drivers;
public:
    void addUser(const string &name, int age, char gender){
        users[name] = {name, age, gender, {0,0}};
    }

    void updateUserLocation(const string &name, Location loc){
        if(users.find(name) != users.end()){
            users[name].location = loc;
        }
    }

    void addDriver(const string &name, int age, char gender, string vehicle, string vehicleNumber, Location loc){
        drivers[name] = {name, age, gender, loc, vehicle, vehicleNumber, true, 0};
    }

    void updateDriverLocation(const string &name, Location loc){
        if(drivers.find(name) != drivers.end()){
            drivers[name].location = loc;
        }
    }

    void changeDriverStatus(const string &name, bool status){
        if(drivers.find(name) != drivers.end()){
            drivers[name].available = status;
        }
    }

    vector<string> findRide(const string &username, Location src, Location dest){
        vector<string> availableDrivers;
        if(users.find(username) == users.end()){
            cout<<"User not found"<<endl;
            return availableDrivers;
        }
        for(auto &driver: drivers){
            if(driver.second.available){
                availableDrivers.push_back(driver.first);
            }
        }

        if(!availableDrivers.empty()){
            cout<<"Available drivers are: ";
            for(auto &name: availableDrivers){
                cout<<name<<endl;
            }
        } else{
            cout<<"No drivers available"<<endl;
        }
        return availableDrivers;
    }

    void chooseRide(const string &username, const string &drivername){
        if(users.find(username) == users.end()){
            cout<<"User not found"<<endl;
            return;
        }
        if(drivers[drivername].available || drivers.find(drivername) == drivers.end()){
            cout<<"Ride not found"<<endl;
            return;
        }

        cout<<"Ride booked with driver"<<drivername<<endl;
    }

    void billing(const string &username, const string &drivername, Location src, Location dest){
        if(drivers.find(drivername) == drivers.end()) return;

        double distance = src.distance(dest);
        double cost = distance * 10;
        drivers[drivername].earnings += cost;
        cout<<"Total cost: "<<cost<<endl;
    }

    void showEarnings(const string &drivername){
        if(drivers.find(drivername) == drivers.end()) return;
        cout<<"Total earnings: "<<drivers[drivername].earnings<<endl;
    }
};

int main(){
    CabBookingSystem cab;
    cab.addUser("Alice", 25, 'F');
    cab.addUser("Bob", 30, 'M');
    cab.addDriver("Charlie", 35, 'M', "SUV", "KA 01 1234", {0,0});
    cab.addDriver("David", 40, 'M', "Sedan", "KA 01 5678", {0,0});
    cab.updateUserLocation("Alice", {1,1});
    cab.updateUserLocation("Bob", {5,5});
    cab.updateDriverLocation("Charlie", {2,2});
    cab.updateDriverLocation("David", {6,6});
    cab.changeDriverStatus("Charlie", false);
    cab.changeDriverStatus("David", true);
    cab.findRide("Alice", {1,1}, {5,5});
    cab.findRide("Bob", {5,5}, {1,1});
    cab.chooseRide("Alice", "David");
    cab.billing("Alice", "David", {1,1}, {5,5});
    cab.showEarnings("David");
    cab.findRide("Bob", {5,5}, {1,1});
    cab.chooseRide("Alice", "David");
    cab.billing("Alice", "David", {1,1}, {5,5});
    cab.showEarnings("David");

    return 0;
}

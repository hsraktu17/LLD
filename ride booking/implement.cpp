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
    void addUser(const string &name, int age, char gender) {
        users[name] = {name, age, gender, {0,0}};
    }

    void updateUserLocation(const string &name, Location loc){
        if(users.find(name) == users.end()){
            cout<<"User not found"<<endl;
            return;
        }else{
            users[name].location = loc;
        }
    }

    void addDriver(const string &drivername, int age, char gender, Location loc, string vehicle, string vehicleNumber){
        drivers[drivername] = {drivername, age, gender, loc, vehicle, vehicleNumber, true, 0};
    }

    void updateDriverLocation(const string &drivername, Location loc){
        if(drivers.find(drivername) == drivers.end()){
            cout<<"Driver not found"<<endl;
            return;
        }else{
            drivers[drivername].location = loc;
        }
    }

    void changeDriverStatus(const string &name, bool status){
        if(drivers.find(name) == drivers.end()){
            cout<<"Driver not found"<<endl;
        }else{
            drivers[name].available = status;
        }
    }

    vector<string> findRide(const string &username, Location src, Location dest){
        vector<string> availableDrivers;
        if(users.find(username) == users.end()){
            cout<<"User not found"<<endl;
            return availableDrivers;
        }

        for(auto &[name, driver]: drivers){
            if(driver.available && driver.location.distance(src) <= 5){
                availableDrivers.push_back(name);
            }
        }

        if(availableDrivers.empty()){
            cout<<"Driver not found"<<endl;
        }else{
            cout<<"Available Drivers are:";
            for(auto &name: availableDrivers){
                cout<<name<<endl;
            }
        }
        return availableDrivers;
    }

    void chooseRide(const string &username, const string &drivername){
        if(users.find(username) == users.end()){
            cout<<"User not found"<<endl;
        }

        if(drivers.find(drivername) == drivers.end()){
            cout<<"Driver not found"<<endl;
        }

        cout<<"Ride started with :"<< drivername<<endl;
    }

    void billing(const string &username, const string &drivername, Location src, Location dest){

        if(drivers.find(drivername) == drivers.end()) return;

        double distance = src.distance(dest);
        double fare = distance * 10;
        drivers[drivername].earnings += fare;
        users[username].location = dest;
        drivers[drivername].location = dest;

        cout<<"Ride fare is:" << fare<<endl;
    }

};

int main(){

    CabBookingSystem app;
    app.addUser("Abhay",23,'M');
    app.addUser("Vikram",29,'M');
    app.addUser("Kriti",22,'F');

    app.updateUserLocation("Abhay",{0,0});
    app.updateUserLocation("Vikram",{10,0});
    app.updateUserLocation("Kriti",{15,6});


    app.addDriver("Driver1", 30, 'M', {11,10}, "Car", "KA01 1234");
    app.addDriver("Driver2", 35, 'M', {10,1}, "Bike", "KA01 1235");
    app.addDriver("Driver3", 40, 'M', {5,3}, "Auto", "KA01 1236");
    
    cout<<"---------------------------------"<<endl;
    app.findRide("Abhay", {0, 0}, {20, 1});
    vector<string> drivers = app.findRide("Vikram", {10, 0}, {15, 3});
    if (!drivers.empty()) {
        app.chooseRide("Vikram", "Driver1");
        app.billing("Vikram", "Driver1", {10, 0}, {15, 3});
        app.updateUserLocation("Vikram", {15, 3});
        app.updateDriverLocation("Driver1", {15, 3});
        app.changeDriverStatus("Driver1", false);
    }

    app.findRide("Kriti", {15, 6}, {20, 4});


    return 0;
}

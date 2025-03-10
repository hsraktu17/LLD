#include<bits/stdc++.h>
#include<math.h>

using namespace std;

struct Location{
    int x, y;
    double distance(const Location &order) const{
        return sqrt(pow(x-order.x,2) + pow(y-order.y,2));
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
        if (users.find(name) != users.end()){
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

        for(auto &[name, driver]: drivers){
            if(driver.available && driver.location.distance(src) <= 5){
                availableDrivers.push_back(name);
            }
        }

        if(!availableDrivers.size()){
            cout<<"No drivers available"<<endl;
            return availableDrivers;
        }else{
            cout<<"Available drivers are: ";
            for(auto &name: availableDrivers){
                cout<< name<<endl;
            }
        }

        return availableDrivers;
    }

    void chooseRide(const string &username, const string &drivername){
        if(users.find(username) == users.end()){
            cout<<"User not found"<<endl;
            return;
        }

        if(drivers.find(drivername) == drivers.end() || !drivers[drivername].available){
            cout<<"Ride not found"<<endl;
            return;
        }

        cout<<"Ride booked with driver"<< drivername<<endl;
    }

    void billing(const string &username, const string &drivername, Location src, Location dest){
     

        if (drivers.find(drivername) == drivers.end()) return;

        double distance = src.distance(dest);
        double fare = distance * 10;
        drivers[drivername].earnings += fare;
        users[username].location = dest;
        drivers[drivername].location = dest;

        cout << "Ride Ended. Bill amount Rs " << fare << endl;
    }

    void find_total_earning() {
        for (auto &[name, driver] : drivers) {
            cout << driver.name << " earned Rs " << driver.earnings << endl;
        }
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


    app.addDriver("Driver1",22,'M',"Swift", "KA-01-1234", {10,1});
    app.addDriver("Driver2",29,'M',"Swift", "KA-01-1234", {11,10});
    app.addDriver("Driver3",24,'M',"Swift", "KA-01-1234", {5,3});


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

    app.find_total_earning();
    return 0;
}

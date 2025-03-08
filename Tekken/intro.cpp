#include<iostream>

using namespace std;

class Human{
    private:
    int age = 10;

    protected:
    int weight;
    public:
    int height;
    string name;

    void getAge(){
        cout<<this->age<<endl;
    }
};

class Male: public Human{

    public:
    char gender = 'M';
    void getGender(){
        cout<<this->gender<<endl;
    }

    int setWeight(int w){
        this->weight = w;
        return this->weight;
    }
};

int main(){
    Male m1;

    m1.getAge();
    m1.getGender();


    int weight = m1.setWeight(70);
    cout<<"weight:";
    cout<<weight<<endl;
}

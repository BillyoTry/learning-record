#include <iostream>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

#define N 100
int f = 0;
string p;

class worker
{
public:
    worker(){}
    void show();
    ~worker(){}
    friend class WorkerList;
private:
    string num,name,sex,edu,address,tel;
    int age,salary;
};

class WorkerList
{
private:
    worker w[N];
    int NUM;//staff num
    int count;//tem num
public:
    void add();
    void sort();
    void search();
    void dele();
    void modify();
    void save();
    void showworker();
    WorkerList(){
     NUM = 0;
     count = 0;
    }
};

void menu()
{
    cout << endl;
    cout << "*******************************************************" << endl;
    cout << "*                                                     *" << endl;
    cout << "*         Welcome to staff management system          *" << endl;
    cout << "*                                                     *" << endl;
    cout << "*              1.Add staff's information              *" << endl;
    cout << "*              2.modify staff's information           *" << endl;
    cout << "*              3.delete staff's information           *" << endl;
    cout << "*              4.sort staff's information             *" << endl;
    cout << "*              5.search staff's information           *" << endl;
    cout << "*              6.show staff's information             *" << endl;
    cout << "*              7.save staff's information             *" << endl;
    cout << "*              8.exit the system                      *" << endl;
    cout << "*                                                     *" << endl;
    cout << "*******************************************************" << endl;
    cout << endl;
}

void domain()
{
    WorkerList wl;
    while(1)
    {
        menu();
        cout << "Please choose the num from 1 ~ 8 " << endl;
        int choice;
        cin >> choice;
        while(choice <= 0 || choice >= 9)
        {
            cout << "Invalid number " << endl;
            cout << "Please press the effective number " << endl;
            cin >> choice;
        }
        switch(choice)
        {
            case 1 :
                wl.add();
                break;
            case 2 :
                wl.modify();
                break;
            case 3 :
                wl.dele();
                break;
            case 4 :
                wl.sort();
                break;
            case 5 :
                wl.search();
                break;
            case 6 :
                wl.showworker();
                break;
            case 7 :
                wl.save();
                break;
            case 8 :
                cout << "Thank you for using the staff management system" << endl;
                cout << " exit the system !" << endl;
                exit(0);
            default:
                break;
        }
        cout << endl;
        cout << "Do you want to return to the main menu ? " << endl;
        cout << "Please press Y/N to choose " << endl;
        char p;
        cin >> p;
        if (p == 'n' || p == 'N')
        {
            cout << "exit the system !! " << endl;
            cout << endl;
            exit(0);
        }
    }
}

void WorkerList::add()
{
    cout << "Please input the staff's information " << endl;
    cout << "job number:"<< endl;
    string job_number;
    cin >> job_number;
    cout << "name\t";
    cout << "sex\t";
    cout << "edu\t";
    cout << "age\t";
    cout << "salary\t";
    cout << "address\t";
    cout << "tel";
    cout << endl;
    for(int i = 0 ; i < NUM ; i++) {
        while ( w[i].num == job_number ) {
            cout << "This job number already exists, please re-enter" << endl;
            cin >> job_number;
        }
    }
    w[NUM].num = job_number;
    cin >> w[NUM].name;
    cin >> w[NUM].sex;
    cin >> w[NUM].edu;
    cin >> w[NUM].age;
    cin >> w[NUM].salary;
    cin >> w[NUM].address;
    cin >> w[NUM].tel;
    ++NUM;
    count = NUM;
}

void worker::show()
{
    cout << "job number\t";
    cout << "name\t";
    cout << "sex\t";
    cout << "edu\t";
    cout << "age\t";
    cout << "salary\t";
    cout << "address\t";
    cout << "tel";
    cout << endl;
    cout << num;
    cout << '\t';
    cout << name;
    cout << '\t';
    cout << sex;
    cout << '\t';
    cout << edu;
    cout << '\t';
    cout << age;
    cout << '\t';
    cout << salary;
    cout << '\t';
    cout << address;
    cout << '\t';
    cout << tel;
    cout << endl;
}

void WorkerList::modify() {
    if(NUM <= 0)
    {
        cout << "No employee information entered, return to the main menu!" << endl;
        menu();
    }
    else
    {
        cout << "staff's information is as follows:" << endl;
        cout << "job number\t";
        cout << "name\t";
        cout << "sex\t";
        cout << "edu\t";
        cout << "age\t";
        cout << "salary\t";
        cout << "address\t";
        cout << "tel";
        cout << endl;
        for(int i = 0 ; i < count ; i++ )
        {
            cout << w[i].num;
            cout << '\t';
            cout << w[i].name;
            cout << '\t';
            cout << w[i].sex;
            cout << '\t';
            cout << w[i].edu;
            cout << '\t';
            cout << w[i].age;
            cout << '\t';
            cout << w[i].salary;
            cout << '\t';
            cout << w[i].address;
            cout << '\t';
            cout << w[i].tel;
            cout << endl;
        }
    }
    cout << "Please input the job number which you want to modify : " << endl;
    string num1;
    cin >> num1;
    int j = 0 , k;
    int flag = 1;
    while(flag)
    {
        for(;j<count;j++)
        {
            if(num1 == w[j].num)
            {
                flag = 0;
                k = j;
                break;
            }
        }
        if(flag)
        {
            cout << "This employee does not exist, please re-enter!" << endl;
            j = 0;
            cin >> num1;
        }
    }
    cout << "The employee information you choose is : " << endl;
    cout << "job number\t" ;
    cout << "name\t" ;
    cout << "sex\t" ;
    cout << "edu\t" ;
    cout << "age\t" ;
    cout << "salary" ;
    cout << "address\t" ;
    cout << "tel" ;
    cout << endl;
    cout << w[k].num;
    cout << '\t';
    cout << w[k].name;
    cout << '\t';
    cout << w[k].sex;
    cout << '\t';
    cout << w[k].edu;
    cout << '\t';
    cout << w[k].age;
    cout << '\t';
    cout << w[k].salary;
    cout << '\t';
    cout << w[k].address;
    cout << '\t';
    cout << w[k].tel << endl;
    cout << endl;
    cout << "*******************************************************" << endl;
    cout << "*                                                     *" << endl;
    cout << "*                 Modify Information                  *" << endl;
    cout << "*                                                     *" << endl;
    cout << "*******************************************************" << endl;
    cout << "*                                                     *" << endl;
    cout << "*                  1.modify job number                *" << endl;
    cout << "*                  2.modify name                      *" << endl;
    cout << "*                  3.modify sex                       *" << endl;
    cout << "*                  4.modify edu                       *" << endl;
    cout << "*                  5.modify age                       *" << endl;
    cout << "*                  6.modify salary                    *" << endl;
    cout << "*                  7.modify address                   *" << endl;
    cout << "*                  8.modify tellphone                 *" << endl;
    cout << "*                  9.renturn to main menu             *" << endl;
    cout << "*                                                     *" << endl;
    cout << "*******************************************************" << endl;
    cout << endl;
    cout << "Please choose the num from 1 ~ 9 " << endl;
    int choice;
    cin >> choice;
    while(choice <= 0 || choice >= 10)
    {
        cout << "Invalid number " << endl;
        cout << "Please press the effective number " << endl;
        cin >> choice;
    }
    switch(choice)
    {
        case 1 :
            cout << "Press the new job number" << endl;
            cin >> w[k].num;
            cout << "The modification information has been saved. Now return to the main menu" << endl;
            break;
        case 2 :
            cout << "Press the new name" << endl;
            cin >> w[k].name;
            cout << "The modification information has been saved. Now return to the main menu" << endl;
            break;
        case 3 :
            cout << "Press the new sex" << endl;
            cin >> w[k].sex;
            cout << "The modification information has been saved. Now return to the main menu" << endl;
            break;
        case 4 :
            cout << "Press the new edu" << endl;
            cin >> w[k].edu;
            cout << "The modification information has been saved. Now return to the main menu" << endl;
            break;
        case 5 :
            cout << "Press the new age" << endl;
            cin >> w[k].age;
            cout << "The modification information has been saved. Now return to the main menu" << endl;
            break;
        case 6 :
            cout << "Press the new salary" << endl;
            cin >> w[k].salary;
            cout << "The modification information has been saved. Now return to the main menu" << endl;
            break;
        case 7 :
            cout << "Press the new address" << endl;
            cin >> w[k].address;
            cout << "The modification information has been saved. Now return to the main menu" << endl;
            break;
        case 8 :
            cout << "Press the new tellphone" << endl;
            cin >> w[k].tel;
            cout << "The modification information has been saved. Now return to the main menu" << endl;
        case 9 :
            cout << "return to main menu" << endl;
            menu();
        default:
            break;
    }
}

void WorkerList::showworker()
{
    if(NUM <= 0)
    {
        cout << "No employee information entered, return to the main menu!" << endl;
        menu();
    }
    else
    {
        cout << "staff's information is as follows:" << endl;
        cout << endl;
        cout << "job number\t";
        cout << "name\t";
        cout << "sex\t";
        cout << "edu\t";
        cout << "age\t";
        cout << "salary\t";
        cout << "address\t";
        cout << "tel";
        cout << endl;
        for(int i = 0 ; i < count ; i++ )
        {
            cout << w[i].num;
            cout << '\t';
            cout << w[i].name;
            cout << '\t';
            cout << w[i].sex;
            cout << '\t';
            cout << w[i].edu;
            cout << '\t';
            cout << w[i].age;
            cout << '\t';
            cout << w[i].salary;
            cout << '\t';
            cout << w[i].address;
            cout << '\t';
            cout << w[i].tel;
            cout << endl;
        }
    }
}
void WorkerList::search()
{
    if(NUM <= 0)
    {
        cout << "No employee information entered, return to the main menu!" << endl;
        menu();
    }
    else
    {
        cout << "Please selete search method! " << endl;
        cout << "*******************************************************" << endl;
        cout << "*                                                     *" << endl;
        cout << "*                 Search Information                  *" << endl;
        cout << "*                                                     *" << endl;
        cout << "*******************************************************" << endl;
        cout << "*                                                     *" << endl;
        cout << "*                  1.search by job number             *" << endl;
        cout << "*                  2.search by name                   *" << endl;
        cout << "*                  3.search by salary                 *" << endl;
        cout << "*                  4.search by edu                    *" << endl;
        cout << "*                  5.renturn to main menu             *" << endl;
        cout << "*                                                     *" << endl;
        cout << "*******************************************************" << endl;
        cout << endl;
        cout << "Please choose the num from 1 ~ 5 " << endl;
        int choice;
        cin >> choice;
        while(choice <= 0 || choice >= 6)
        {
            cout << "Invalid number " << endl;
            cout << "Please press the effective number " << endl;
            cin >> choice;
        }
        if(choice == 1)
        {
            string num2;
            cout << "Please input the job number which you want to search" << endl;
            cin >> num2;
            int j = 0,k;
            int flag = 1;
            while(flag)
            {
                for(; j < count ; j++)
                {
                    if(num2 == w[j].num)
                    {
                        flag = 0;
                        k = j;
                        cout << "staff's information is as follows" << endl;
                        cout << "job number\t";
                        cout << "name\t";
                        cout << "sex\t";
                        cout << "edu\t";
                        cout << "age\t";
                        cout << "salary\t";
                        cout << "address\t";
                        cout << "tel";
                        cout << endl;
                        cout << w[k].num;
                        cout << '\t';
                        cout << w[k].name;
                        cout << '\t';
                        cout << w[k].sex;
                        cout << '\t';
                        cout << w[k].edu;
                        cout << '\t';
                        cout << w[k].age;
                        cout << '\t';
                        cout << w[k].salary;
                        cout << '\t';
                        cout << w[k].address;
                        cout << '\t';
                        cout << w[k].tel;
                        cout << endl;
                        break;
                    }
                }
                if(flag)
                {
                    flag = 0;
                    cout << "This employee does not exist, please re-enter!" << endl;
                    break;
                }
            }
        }
        else if(choice == 2)
        {
            string name2;
            cout << "Please input the name which you want to search" << endl;
            cin >> name2;
            int j = 0,k;
            int flag = 1;
            while(flag)
            {
                for(; j < count ; j++)
                {
                    if(name2 == w[j].name)
                    {
                        flag = 0;
                        k = j;
                        cout << "staff's information is as follows" << endl;
                        cout << "job number\t";
                        cout << "name\t";
                        cout << "sex\t";
                        cout << "edu\t";
                        cout << "age\t";
                        cout << "salary\t";
                        cout << "address\t";
                        cout << "tel";
                        cout << endl;
                        cout << w[k].num;
                        cout << '\t';
                        cout << w[k].name;
                        cout << '\t';
                        cout << w[k].sex;
                        cout << '\t';
                        cout << w[k].edu;
                        cout << '\t';
                        cout << w[k].age;
                        cout << '\t';
                        cout << w[k].salary;
                        cout << '\t';
                        cout << w[k].address;
                        cout << '\t';
                        cout << w[k].tel;
                        cout << endl;
                        break;
                    }
                }
                if(flag)
                {
                    flag = 0;
                    cout << "This employee does not exist, please re-enter!" << endl;
                    break;
                }
            }
        }
        else if(choice == 3)
        {
            int salary2;
            cout << "Please input the salary which you want to search" << endl;
            cin >> salary2;
            int j = 0,k;
            int flag = 1;
            while(flag)
            {
                for(; j < count ; j++)
                {
                    if(salary2 == w[j].salary)
                    {
                        flag = 0;
                        k = j;
                        cout << "staff's information is as follows" << endl;
                        cout << "job number\t";
                        cout << "name\t";
                        cout << "sex\t";
                        cout << "edu\t";
                        cout << "age\t";
                        cout << "salary\t";
                        cout << "address\t";
                        cout << "tel";
                        cout << endl;
                        cout << w[k].num;
                        cout << '\t';
                        cout << w[k].name;
                        cout << '\t';
                        cout << w[k].sex;
                        cout << '\t';
                        cout << w[k].edu;
                        cout << '\t';
                        cout << w[k].age;
                        cout << '\t';
                        cout << w[k].salary;
                        cout << '\t';
                        cout << w[k].address;
                        cout << '\t';
                        cout << w[k].tel;
                        cout << endl;
                        break;
                    }
                }
                if(flag)
                {
                    flag = 0;
                    cout << "This employee does not exist, please re-enter!" << endl;
                    break;
                }
            }
        }
        else if(choice ==4)
        {
            string edu2;
            cout << "Please input the edu which you want to search" << endl;
            cin >> edu2;
            int j = 0,k;
            int flag = 1;
            while(flag)
            {
                for(; j < count ; j++)
                {
                    if(edu2 == w[j].edu)
                    {
                        flag = 0;
                        k = j;
                        cout << "staff's information is as follows" << endl;
                        cout << "job number\t";
                        cout << "name\t";
                        cout << "sex\t";
                        cout << "edu\t";
                        cout << "age\t";
                        cout << "salary\t";
                        cout << "address\t";
                        cout << "tel";
                        cout << endl;
                        cout << w[k].num;
                        cout << '\t';
                        cout << w[k].name;
                        cout << '\t';
                        cout << w[k].sex;
                        cout << '\t';
                        cout << w[k].edu;
                        cout << '\t';
                        cout << w[k].age;
                        cout << '\t';
                        cout << w[k].salary;
                        cout << '\t';
                        cout << w[k].address;
                        cout << '\t';
                        cout << w[k].tel;
                        cout << endl;
                        break;
                    }
                }
                if(flag)
                {
                    flag = 0;
                    cout << "This employee does not exist, please re-enter!" << endl;
                    break;
                }
            }
        }
        else if(choice == 5)
        {
            menu();
        }
    }
}

void WorkerList::sort()
{
    if(NUM <= 0)
    {
        cout << "No employee information entered, return to the main menu!" << endl;
        menu();
    }
    else
    {
        cout << "Please select sorting method!" << endl;
        cout << "*******************************************************" << endl;
        cout << "*                                                     *" << endl;
        cout << "*                   Sort Information                  *" << endl;
        cout << "*                                                     *" << endl;
        cout << "*******************************************************" << endl;
        cout << "*                                                     *" << endl;
        cout << "*         1.sort by job number(Ascending order)       *" << endl;
        cout << "*         2.sort by job number(Descending order)      *" << endl;
        cout << "*         3.sort by name(Ascending order)             *" << endl;
        cout << "*         4.sort by name(Descending order)            *" << endl;
        cout << "*         5.sort by salary(Ascending order)           *" << endl;
        cout << "*         6.sort by salary(Descending order)          *" << endl;
        cout << "*         7.renturn to main menu                      *" << endl;
        cout << "*                                                     *" << endl;
        cout << "*******************************************************" << endl;
        cout << endl;
        cout << "Please choose the num from 1 ~ 7 " << endl;
        int choice;
        cin >> choice;
        while(choice <= 0 || choice >= 8)
        {
            cout << "Invalid number " << endl;
            cout << "Please press the effective number " << endl;
            cin >> choice;
        }
        switch(choice)
        {
            case 1 :
                for(int i = 0;i < count - 1;i++)
                {
                    for (int j = 0;j < count - 1 - i;j++)
                    {
                        if(w[j].num > w[j+1].num)
                        {
                            string temp;
                            temp = w[j].num;
                            w[j].num = w[j+1].num;
                            w[j+1].num = temp;
                        }
                    }
                }
                cout << "The information in ascending order by job number is as follows: " <<endl;
                cout << "staff's information is as follows" << endl;
                cout << "job number\t";
                cout << "name\t";
                cout << "sex\t";
                cout << "edu\t";
                cout << "age\t";
                cout << "salary\t";
                cout << "address\t";
                cout << "tel";
                cout << endl;
                for(int i = 0;i < count ;i++)
                {
                    cout << w[i].num;
                    cout << '\t';
                    cout << w[i].name;
                    cout << '\t';
                    cout << w[i].sex;
                    cout << '\t';
                    cout << w[i].edu;
                    cout << '\t';
                    cout << w[i].age;
                    cout << '\t';
                    cout << w[i].salary;
                    cout << '\t';
                    cout << w[i].address;
                    cout << '\t';
                    cout << w[i].tel ;
                    cout << endl;
                }
                break;
            case 2 :
                for(int i = 0 ;i < count - 1;i++)
                {
                    for(int j = 0;j < count - 1 - i;j++)
                    {
                        if(w[j].num < w[j+1].num)
                        {
                            string temp;
                            temp = w[j].num;
                            w[j].num = w[j+1].num;
                            w[j+1].num = temp;
                        }
                    }
                }
                cout << "The information in descending order by job number is as follows: " <<endl;
                cout << "staff's information is as follows" << endl;
                cout << "job number\t";
                cout << "name\t";
                cout << "sex\t";
                cout << "edu\t";
                cout << "age\t";
                cout << "salary\t";
                cout << "address\t";
                cout << "tel";
                cout << endl;
                for(int i = 0;i < count ;i++)
                {
                    cout << w[i].num;
                    cout << '\t';
                    cout << w[i].name;
                    cout << '\t';
                    cout << w[i].sex;
                    cout << '\t';
                    cout << w[i].edu;
                    cout << '\t';
                    cout << w[i].age;
                    cout << '\t';
                    cout << w[i].salary;
                    cout << '\t';
                    cout << w[i].address;
                    cout << '\t';
                    cout << w[i].tel ;
                    cout << endl;
                }
                break;
            case 3 :
                for(int i = 0;i < count - 1;i++)
                {
                    for(int j = 0; j < count - 1 -i;j++)
                    {
                        if(w[j].name > w[j+1].name)
                        {
                            string temp;
                            temp = w[j].name;
                            w[j].name = w[j+1].name;
                            w[j+1].name = temp;
                        }
                    }
                }
                cout << "The information in ascending order by name is as follows: " <<endl;
                cout << "staff's information is as follows" << endl;
                cout << "job number\t";
                cout << "name\t";
                cout << "sex\t";
                cout << "edu\t";
                cout << "age\t";
                cout << "salary\t";
                cout << "address\t";
                cout << "tel";
                cout << endl;
                for(int i = 0;i < count ;i++)
                {
                    cout << w[i].num;
                    cout << '\t';
                    cout << w[i].name;
                    cout << '\t';
                    cout << w[i].sex;
                    cout << '\t';
                    cout << w[i].edu;
                    cout << '\t';
                    cout << w[i].age;
                    cout << '\t';
                    cout << w[i].salary;
                    cout << '\t';
                    cout << w[i].address;
                    cout << '\t';
                    cout << w[i].tel ;
                    cout << endl;
                }
                break;
            case 4 :
                for(int i = 0;i < count - 1;i++)
                {
                    for(int j = 0; j < count - 1 -i;j++)
                    {
                        if(w[j].name < w[j+1].name)
                        {
                            string temp;
                            temp = w[j].name;
                            w[j].name = w[j+1].name;
                            w[j+1].name = temp;
                        }
                    }
                }
                cout << "The information in descending order by name is as follows: " <<endl;
                cout << "staff's information is as follows" << endl;
                cout << "job number\t";
                cout << "name\t";
                cout << "sex\t";
                cout << "edu\t";
                cout << "age\t";
                cout << "salary\t";
                cout << "address\t";
                cout << "tel";
                cout << endl;
                for(int i = 0;i < count ;i++)
                {
                    cout << w[i].num;
                    cout << '\t';
                    cout << w[i].name;
                    cout << '\t';
                    cout << w[i].sex;
                    cout << '\t';
                    cout << w[i].edu;
                    cout << '\t';
                    cout << w[i].age;
                    cout << '\t';
                    cout << w[i].salary;
                    cout << '\t';
                    cout << w[i].address;
                    cout << '\t';
                    cout << w[i].tel ;
                    cout << endl;
                }
                break;
            case 5 :
                for(int i = 0;i < count - 1;i++)
                {
                    for(int j = 0; j < count - 1 -i;j++)
                    {
                        if(w[j].salary > w[j+1].salary)
                        {
                            int temp;
                            temp = w[j].salary;
                            w[j].salary = w[j+1].salary;
                            w[j+1].salary = temp;
                        }
                    }
                }
                cout << "The information in ascending order by salary is as follows: " <<endl;
                cout << "staff's information is as follows" << endl;
                cout << "job number\t";
                cout << "name\t";
                cout << "sex\t";
                cout << "edu\t";
                cout << "age\t";
                cout << "salary\t";
                cout << "address\t";
                cout << "tel";
                cout << endl;
                for(int i = 0;i < count ;i++)
                {
                    cout << w[i].num;
                    cout << '\t';
                    cout << w[i].name;
                    cout << '\t';
                    cout << w[i].sex;
                    cout << '\t';
                    cout << w[i].edu;
                    cout << '\t';
                    cout << w[i].age;
                    cout << '\t';
                    cout << w[i].salary;
                    cout << '\t';
                    cout << w[i].address;
                    cout << '\t';
                    cout << w[i].tel ;
                    cout << endl;
                }
                break;
            case 6 :
                for(int i = 0;i < count - 1;i++)
                {
                    for(int j = 0; j < count - 1 -i;j++)
                    {
                        if(w[j].salary < w[j+1].salary)
                        {
                            int temp;
                            temp = w[j].salary;
                            w[j].salary = w[j+1].salary;
                            w[j+1].salary = temp;
                        }
                    }
                }
                cout << "The information in descending order by salary is as follows: " <<endl;
                cout << "staff's information is as follows" << endl;
                cout << "job number\t";
                cout << "name\t";
                cout << "sex\t";
                cout << "edu\t";
                cout << "age\t";
                cout << "salary\t";
                cout << "address\t";
                cout << "tel";
                cout << endl;
                for(int i = 0;i < count ;i++)
                {
                    cout << w[i].num;
                    cout << '\t';
                    cout << w[i].name;
                    cout << '\t';
                    cout << w[i].sex;
                    cout << '\t';
                    cout << w[i].edu;
                    cout << '\t';
                    cout << w[i].age;
                    cout << '\t';
                    cout << w[i].salary;
                    cout << '\t';
                    cout << w[i].address;
                    cout << '\t';
                    cout << w[i].tel ;
                    cout << endl;
                }
                break;
            case 7 :
                menu();
                break;
            default :
                break;
        }
    }
}

void WorkerList::dele()
{
    if(NUM <= 0)
    {
        cout << "No employee information entered, return to the main menu!" << endl;
        menu();
    }
    else
    {
        cout << "staff's information is as follows" << endl;
        cout << "job number\t";
        cout << "name\t";
        cout << "sex\t";
        cout << "edu\t";
        cout << "age\t";
        cout << "salary\t";
        cout << "address\t";
        cout << "tel";
        cout << endl;
        for(int i = 0;i < count ;i++)
        {
            cout << w[i].num;
            cout << '\t';
            cout << w[i].name;
            cout << '\t';
            cout << w[i].sex;
            cout << '\t';
            cout << w[i].edu;
            cout << '\t';
            cout << w[i].age;
            cout << '\t';
            cout << w[i].salary;
            cout << '\t';
            cout << w[i].address;
            cout << '\t';
            cout << w[i].tel ;
            cout << endl;
        }
        cout << "Please enter the job number of the employee who needs to delete the information!" << endl;
        string num3;
        cin >> num3;
        int j = 0,k;
        int flag = 1;
        while(flag)
        {
            for(;j < count;j++)
            {
                if(num3 == w[j].num)
                {
                    flag = 0;
                    k = j;
                    break;
                }
            }
            if(flag)
            {
                cout << "This job number didn't exists, please re-enter" << endl;
                j = 0;
                cin >> num3;
            }
        }
        cout << "The information of the employee you choose is: " << endl;
        cout << "job number\t";
        cout << "name\t";
        cout << "sex\t";
        cout << "edu\t";
        cout << "age\t";
        cout << "salary\t";
        cout << "address\t";
        cout << "tel";
        cout << endl;
        cout << w[k].num;
        cout << '\t';
        cout << w[k].name;
        cout << '\t';
        cout << w[k].sex;
        cout << '\t';
        cout << w[k].edu;
        cout << '\t';
        cout << w[k].age;
        cout << '\t';
        cout << w[k].salary;
        cout << '\t';
        cout << w[k].address;
        cout << '\t';
        cout << w[k].tel ;
        cout << endl;
        cout << "Press enter Y/y to ensure !" << endl;
        cout << "Or enter N/n to return to main menu !" << endl;
        string p ;
        cin >> p;
        while(1)
        {
            if(p == "y" || p == "Y")
            {
                cout << "Staff's information has been deleted!" << endl;
                for(int i = 0;i < count;i++)
                    if(w[i].num == num3)
                        int j = i;
                    for(;j <= count - 1;j++)
                    w[j] = w[j+1];
                        count--;
                break;
            }
            else if(p == "n" || p == "N")
            {
               menu();
            } else{
                cout << "Invalid input,please enter again!" << endl;
                cin >> p;
            }
        }
    }
}

void WorkerList::save()
{
    if(NUM <= 0)
    {
        cout << "No employee information entered, return to the main menu!" << endl;
        menu();
    } else{
        ofstream  fout;
        fout.open("D:\\WorkerInformationList.txt",ios::out);
        cout << "Saving....." << endl;
        cout << "Save successful !!" << endl;
        fout << "The saved employee information is as follows: " <<endl;
        fout << "job number\t";
        fout << "name\t";
        fout << "sex\t";
        fout << "edu\t";
        fout << "age\t";
        fout << "salary\t";
        fout << "address\t";
        fout << "tel";
        fout << endl;
        for(int i = 0;i < count;i++)
        {
            fout << w[i].num;
            fout << '\t';
            fout << w[i].name;
            fout << '\t';
            fout << w[i].sex;
            fout << '\t';
            fout << w[i].edu;
            fout << '\t';
            fout << w[i].age;
            fout << '\t';
            fout << w[i].salary;
            fout << '\t';
            fout << w[i].address;
            fout << '\t';
            fout << w[i].tel ;
            fout << endl;
        }
        cout << "return to main menu: " << endl;
        menu();
        fout.close();
    }
}

int main(void)
{
    domain();
    return 0;
}

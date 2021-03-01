#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <set>

using namespace std;

int main(int, char** argv) {

    string query = argv[1];

	if (query == "q1") {
        ifstream inFile;

        string customerFile = argv[2];
        string zonecostFile = argv[3];

        string zoneID;

        inFile.open(argv[3]);

        if (!inFile) {
            cout << "Unable to open file datafile.txt";
            exit(1);
        }

        string line;

        getline(inFile, line);
        getline(inFile, line);
        while (getline(inFile, line)) {
            if (line.substr(7, 7) == "Toronto") {
                zoneID = line.substr(0, 6);
            } 
        }
        
        inFile.close();

        inFile.open(argv[2]);

        if (!inFile) {
            cout << "Unable to open file datafile.txt";
            exit(1);
        }

        getline(inFile, line);
        getline(inFile, line);
        while (getline(inFile, line)) {
            if (line.substr(243, 1) == "1" and line.substr(135, 6) == zoneID) {
                cout << line.substr(42, 20) << endl;
            } 
        }
    }

    if (query == "q2") {
        ifstream inFile;

        string lineitemFile = argv[2];
        string productsFile = argv[3];

        map<string, set<string>> userPurchase;

        inFile.open(argv[2]);

        if (!inFile) {
            cout << "Unable to open file datafile.txt";
            exit(1);
        }

        string line;

        getline(inFile, line);
        getline(inFile, line);
        while (getline(inFile, line)) {
            if (userPurchase.count(line.substr(0, 20))) {
                userPurchase[line.substr(0, 20)].insert(line.substr(41, 20));
            } else {
                set<string> s;
                s.insert(line.substr(41, 20));
                userPurchase.insert(pair<string, set<string>>(line.substr(0, 20), s));
            }
        }

        map<string, int> countPurchase;

        map<string, set<string>>::iterator mapIter;
        set<string>::iterator setIter;

        for(mapIter = userPurchase.begin(); mapIter != userPurchase.end(); mapIter++){
            for(setIter = mapIter->second.begin(); setIter != mapIter->second.end(); setIter++){
                if (countPurchase.count(*setIter)){
                    countPurchase[*setIter]++;
                } else {
                    countPurchase.insert(pair<string, int>(*setIter, 1));
                }
            }
        }
        
        inFile.close();

        inFile.open(argv[3]);

        if (!inFile) {
            cout << "Unable to open file datafile.txt";
            exit(1);
        }

        getline(inFile, line);
        getline(inFile, line);
        while (getline(inFile, line)) {
            if (countPurchase.count(line.substr(0, 20)) and countPurchase[line.substr(0, 20)] >= 2) {
                cout << line.substr(0, 20) << line.substr(32, 50) << endl;
            }
        }

    }
}
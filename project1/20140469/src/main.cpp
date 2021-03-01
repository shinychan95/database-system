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

    // <your_binary> q1 <customer.file> <zonecost.file>
	if (query == "q1") {
        ifstream inFile;

        string customerFile = argv[2];
        string zonecostFile = argv[3];

        string zoneID;

        inFile.open(zonecostFile);

        if (!inFile) {
            cout << "Unable to open zonecost file" << endl;
            exit(1);
        }

        string line;

        getline(inFile, line); // column names
        getline(inFile, line); // hyphens
        while (getline(inFile, line)) {
            if (line.substr(7, 7) == "Toronto") {
                zoneID = line.substr(0, 6);
                break;
            } 
        }
        
        inFile.close();


        inFile.open(customerFile);

        if (!inFile) {
            cout << "Unable to open customer file" << endl;
            exit(1);
        }

        getline(inFile, line); // column names
        getline(inFile, line); // hyphens
        while (getline(inFile, line)) {
            if (line.substr(243, 1) == "1" and line.substr(135, 6) == zoneID) {
                cout << line.substr(42, 20) << endl;
            } 
        }

        return 0;
    }
    // <your_binary> q2 <lineitem.file> <products.file>
    else if (query == "q2") {
        ifstream inFile;

        string lineitemFile = argv[2];
        string productsFile = argv[3];

        map<string, set<string>> productPurchased; // <BARCODE, set<UNAME>>
        map<string, string> productInformaton;     // <BARCODE, PRODDESC>

        inFile.open(lineitemFile);

        if (!inFile) {
            cout << "Unable to open lineitem file" << endl;
            exit(1);
        }

        string line;

        getline(inFile, line);
        getline(inFile, line);
        while (getline(inFile, line)) {
            if (productPurchased.find(line.substr(41, 20)) == productPurchased.end()) {
                set<string> s;
                s.insert(line.substr(0, 20));
                productPurchased.insert(pair<string, set<string>>(line.substr(41, 20), s));
            }
            else {
                productPurchased[line.substr(41, 20)].insert(line.substr(0, 20));
            }
        }
        
        inFile.close();


        inFile.open(productsFile);

        if (!inFile) {
            cout << "Unable to open products file" << endl;
            exit(1);
        }

        getline(inFile, line);
        getline(inFile, line);
        while (getline(inFile, line)) {
            productInformaton.insert(pair<string, string>(line.substr(0, 20), line.substr(32, 50)));
        }

        inFile.close();


        map<string, set<string>>::iterator mapIter;

        for(mapIter = productPurchased.begin(); mapIter != productPurchased.end(); mapIter++){
            string BARCODE = mapIter->first;

            if (mapIter->second.size() > 1) {
                cout << BARCODE << productInformaton[BARCODE] << endl;
            }
        }

        return 0;
    }
    else {
        cout << "Not available query string" << endl;
        exit(1);
    }
}
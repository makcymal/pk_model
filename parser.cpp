#include <bits/stdc++.h>

using namespace std;

const string PATH = "pk_model/";


void ignorelines(ifstream &input, int n = 1) {
    string tmpstr;
    for (int i = 0; i < n; ++i)
        getline(input, tmpstr);
}


void ignorewhile(ifstream &input, string stop) {
    string tmpstr;
    while (input >> tmpstr)
        if (tmpstr == stop)
            break;
}


pair<string,string> parse_edge(string &edge) {
    string from = edge.substr(0, 4), to = edge.substr(5, 4);
    return {from, to};
}


void read_abbrev(map<string, string> &abbrev) {
    ifstream abbrev_file;
    abbrev_file.open(PATH + "abbreviations.txt");
    string full_name, short_name;

    while (abbrev_file >> full_name) {
        abbrev_file >> short_name;
        abbrev[short_name] = full_name;
    }

    abbrev_file.close();
}


int main() {
    map<string, string> abbrev;
    read_abbrev(abbrev);

    ofstream flows_file;
    flows_file.open("flows.csv");
    flows_file << "Откуда\tкуда\tпоток\tограничение\n";

    ifstream table_file;
    table_file.open("out.presol");
    ignorewhile(table_file, "Column");
    ignorelines(table_file, 2);

    string edge, tmpstr;
    int flow, bound;
    for (int no = 1; table_file >> tmpstr; ++no) {
        if (tmpstr != to_string(no)) break;

        getline(table_file, tmpstr);
        stringstream ss(tmpstr);
        ss >> edge;
        ss >> tmpstr;
        ss >> flow;
        ss >> tmpstr;
        ss >> bound;

        auto [from, to] = parse_edge(edge);
        if (flow != 0)
           flows_file << abbrev[from] << '\t' << abbrev[to] << '\t' << flow << '\t' << bound << endl;
    }

    flows_file.close();
    table_file.close();
    return 0;
}
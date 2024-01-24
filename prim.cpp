#include <bits/stdc++.h>

using namespace std;


void get_input(vector<string> &verts, map<string, int> &popul, map<string, int> &index, vector<vector<int>> &dist) {
    string str_input;
    int int_input;

    ifstream verts_popul, edges;
    verts_popul.open("pk_model/graph_popul.txt");
    edges.open("pk_model/graph_edges.txt");

    // get vertex
    while (verts_popul >> str_input) {
        verts.push_back(str_input);
        // get population
        verts_popul >> int_input;
        popul[verts.back()] = int_input;
    }

    // sort by population in descending order
    sort(verts.begin(), verts.end(), [&](string &a, string &b) { return popul[a] > popul[b]; });

    // get vertices sorted indices
    for (int i = 0; i < verts.size(); ++i)
        index[verts[i]] = i;

    dist.resize(verts.size());
    for (auto &row: dist)
        row.resize(verts.size(), 0);

    while (edges >> str_input) {
        int v1 = index[str_input];
        edges >> str_input;
        int v2 = index[str_input];
        edges >> int_input;
        dist[v1][v2] = dist[v2][v1] = int_input;
    }

    verts_popul.close();
    edges.close();
}


void get_abbrev(map<string, string> &abbrev) {
    ifstream abbrev_file;
    abbrev_file.open("pk_model/abbreviations.txt");
    string full_name, short_name;

    while (abbrev_file >> full_name) {
        abbrev_file >> short_name;
        abbrev[full_name] = short_name;
    }

    abbrev_file.close();
}


void write_verts_order(vector<string> &verts) {
    ofstream verts_order;
    verts_order.open("verts_order.txt");
    for (int i = 0; i < verts.size(); ++i)
        verts_order << i << '\t' << verts[i] << endl;
    verts_order.close();
}


int main() {
    // vertices and its populations
    vector<string> verts;
    map<string, int> popul, index;
    // matrix of edges distances
    vector<vector<int>> dist;

    get_input(verts, popul, index, dist);

    map<string, string> abbrev;
    get_abbrev(abbrev);

    const int N = verts.size();
    string src = "Владивосток", dst = "Находка";
    int src_idx = index[src], dst_idx = index[dst];

    // assuming 1km of lane can hold up to 150 cars
    ofstream statement;
    statement.open("statement.mps");

    statement << "NAME          PK_ROADS\n";
    statement << "ROWS\n";

    // function to minimize
    statement << " N\tTARGET\n";

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] > 0) {
                statement << " L  MAX_" << abbrev[verts[r]] << '_' << abbrev[verts[c]] << '\n';
            }
        }
    }

    for (int i = 0; i < N; ++i) {
        if (i != src_idx and i != dst_idx)
            statement << " E  INOUT_" << abbrev[verts[i]] << '\n';
    }
    statement << " E  ENDS_" << abbrev[src] << '_' << abbrev[dst] << '\n';

    statement << "COLUMNS\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] > 0) {
                string var = "\t" + abbrev[verts[r]] + '_' + abbrev[verts[c]];
                statement << var << "\t\tTARGET\t\t\t\t-1\n";
                statement << var << "\t\tMAX_" << abbrev[verts[r]] << '_' << abbrev[verts[c]] << "\t\t1\n";
                
                if (c == src_idx or r == dst_idx) continue;
                
                if (r != src_idx and c != dst_idx){
                    statement << var << "\t\tINOUT_" << abbrev[verts[r]] << "\t\t\t-1\n";
                    statement << var << "\t\tINOUT_" << abbrev[verts[c]] << "\t\t\t1\n";
                } else if (r == src_idx) {
                    statement << var << "\t\tENDS_" << abbrev[src] << '_' << abbrev[dst] << "\t\t-1\n";
                    statement << var << "\t\tINOUT_" << abbrev[verts[c]] << "\t\t\t1\n";
                } else if (c == dst_idx) {
                    statement << var << "\t\tINOUT_" << abbrev[verts[r]] << "\t\t\t-1\n";
                    statement << var << "\t\tENDS_" << abbrev[src] << '_' << abbrev[dst] << "\t\t1\n";
                }
            }
        }
    }

    statement << "RHS\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] > 0) {
                int cap = ((r < 10 or c < 10) ? 3 : 2) * 150 * dist[r][c];
                statement << "\tMAX\t\tMAX_" << abbrev[verts[r]] << '_' << abbrev[verts[c]] << "\t\t" << cap << '\n';
            }
        }
    }

    statement << "ENDATA\n";

    return 0;
}
#include <bits/stdc++.h>

using namespace std;

const string PATH = "pk_model/";
// const string PATH = "exp_model_3/";


void read_graph(vector<string> &verts, map<string, int> &popul, map<string, int> &index, vector<vector<int>> &dist) {
    string str_input;
    int int_input;

    ifstream popul_file, edges_file;
    popul_file.open(PATH + "graph_popul.txt");
    edges_file.open(PATH + "graph_edges.txt");

    // get vertex
    while (popul_file >> str_input) {
        verts.push_back(str_input);
        // get population
        popul_file >> int_input;
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

    while (edges_file >> str_input) {
        int v1 = index[str_input];
        edges_file >> str_input;
        int v2 = index[str_input];
        edges_file >> int_input;

        dist[v1][v2] = dist[v2][v1] = int_input;
        // dist[v1][v2] = int_input;
    }

    popul_file.close();
    edges_file.close();
}


void read_abbrev(map<string, string> &abbrev) {
    ifstream abbrev_file;
    abbrev_file.open(PATH + "abbreviations.txt");
    string full_name, short_name;

    while (abbrev_file >> full_name) {
        abbrev_file >> short_name;
        abbrev[full_name] = short_name;
    }

    abbrev_file.close();
}


void read_src_dst(string &src, string &dst) {
    ifstream src_dst_file;
    src_dst_file.open(PATH + "src_dst.txt");
    src_dst_file >> src >> dst;
    src_dst_file.close();
}


void write_verts_order(vector<string> &verts) {
    ofstream verts_order;
    verts_order.open(PATH + "verts_order.txt");
    for (int i = 0; i < verts.size(); ++i)
        verts_order << i << '\t' << verts[i] << endl;
    verts_order.close();
}


int main() {
    // vertices and its populations
    vector<string> verts;
    map<string, int> popul, index;
    // matrix of edges_file distances
    vector<vector<int>> dist;

    read_graph(verts, popul, index, dist);
    write_verts_order(verts);

    const int N = verts.size();

    map<string, string> abbrev;
    read_abbrev(abbrev);

    string src, dst;
    read_src_dst(src, dst);
    int src_idx = index[src], dst_idx = index[dst];

    ofstream mps;
    mps.open("statement.mps");

    mps << "NAME          PK_ROADS\n";
    mps << "ROWS\n";

    // function to minimize
    mps << " N\tMAXFLOW\n";

    for (int i = 0; i < N; ++i) {
        if (i != src_idx and i != dst_idx)
            mps << " E  IO_" << abbrev[verts[i]] << '\n';
    }
    mps << " E  SRC_DST\n";

    mps << "COLUMNS\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] > 0 and c != src_idx and r != dst_idx) {
                string var = "\t" + abbrev[verts[r]] + '_' + abbrev[verts[c]] + "\t\t";

                if (r == src_idx){
                    mps << var << "MAXFLOW\t\t\t1\n";
                    if (c == dst_idx) continue;
                }
                
                if (r != src_idx and c != dst_idx){
                    mps << var << "IO_" << abbrev[verts[r]] << "\t\t\t-1\n";
                    mps << var << "IO_" << abbrev[verts[c]] << "\t\t\t1\n";
                } else if (r == src_idx) {
                    mps << var << "SRC_DST" << "\t\t\t-1\n";
                    mps << var << "IO_" << abbrev[verts[c]] << "\t\t\t1\n";
                } else if (c == dst_idx) {
                    mps << var << "IO_" << abbrev[verts[r]] << "\t\t\t-1\n";
                    mps << var << "SRC_DST" << "\t\t\t1\n";
                }
            }
        }
    }

    mps << "BOUNDS\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] > 0 and c != src_idx and r != dst_idx) {
                int cap = dist[r][c];
                // int cap = ((r < 10 or c < 10) ? 3 : 2) * 150 * dist[r][c];
                mps << " UP FLOW\t\t\t" << abbrev[verts[r]] << '_' << abbrev[verts[c]] << "\t\t" << cap << '\n';
                mps << " LO FLOW\t\t\t" << abbrev[verts[r]] << '_' << abbrev[verts[c]] << "\t\t0\n";
            }
        }
    }

    mps << "ENDATA\n";

    return 0;
}
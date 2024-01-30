#include <bits/stdc++.h>

using namespace std;

// path to directory with PK graph sources
const string MODELPATH = "pk_model/";
// filename with LP problem statement; used by glpsol
const string MPSFILE = "statement.mps";
// filename with the glpsol solution
const string OUTPUTFILE = "solution.out";
// filename with the resulting graph of flows, used by graphviz
const string DOTFILE = "flows.dot";


using pii = pair<int, int>;
using vi = vector<int>;
using vvi = vector<vector<int>>;
using pss = pair<string, string>;
using vs = vector<string>;
using mss = map<string, string>;
using msi = map<string, int>;


double sigmd(double x) {
    return 1. / (1. + exp(-(x / 20000.))) * 10000.;
}


// each vertex name is assosiated with its abbreviation consisting of exactly 4 letters
// they are listed in 'MODELPATH/abbreviations.txt" in following format "vertex_name abbreviation"
// to_abbrev maps vertex name to its abbreviation, from_abbrev - vice versa
void read_abbrev(mss &to_abbrev, mss &from_abbrev) {
    ifstream abbrev_file;
    abbrev_file.open(MODELPATH + "abbreviations.txt");
    // full vertex name and its abbreviation
    string full_name, short_name;

    // reading list of abbreviations word by word
    // the first word in each line is full vertex name
    while (abbrev_file >> full_name) {
        // the second one is its abbreviation
        abbrev_file >> short_name;
        // map them together
        to_abbrev[full_name] = short_name;
        from_abbrev[short_name] = full_name;
    }

    abbrev_file.close();
}


// read all the vertices and edges of PK graph
// verts is the list of vertices full names
// popul contains population numbers by vertex full name
// index maps vertex full name to its index in verts
// dist is the matrix of distances where 0 means there is no straight road
void read_graph(vs &verts, msi &popul, msi &index, vvi &dist) {
    string str_input;
    int int_input;

    ifstream popul_file, edges_file;
    popul_file.open(MODELPATH + "graph_popul.txt");
    edges_file.open(MODELPATH + "graph_edges.txt");

    // first read popul_file that contains vertices full names and populations
    // the first word is the vertex name
    while (popul_file >> str_input) {
        // the second one is its population
        popul_file >> int_input;
        // fill verts and popul with the given values
        verts.push_back(str_input);
        popul[verts.back()] = int_input;
    }

    // sort by population in descending order
    sort(verts.begin(), verts.end(), [&](string &a, string &b) {
        return popul[a] > popul[b]; 
    });

    // get vertices sorted indices
    for (int i = 0; i < verts.size(); ++i)
        index[verts[i]] = i;
    
    // init dist matrix with zeros
    dist.resize(verts.size());
    for (auto &row: dist)
        row.resize(verts.size(), 0);

    // finally read edges and they distances filling with them dist matrix
    // edges is given in the following format 'vertex_from vertex_to distance' line by line
    while (edges_file >> str_input) {
        int v1 = index[str_input];
        edges_file >> str_input;
        int v2 = index[str_input];
        edges_file >> int_input;

        dist[v1][v2] = dist[v2][v1] = int_input;
    }

    popul_file.close();
    edges_file.close();
}


vvi count_bandwidth(vvi &dist) {
    vvi bandwidth;
    bandwidth.resize(dist.size());
    for (int r = 0; r < dist.size(); ++r) {
        bandwidth[r].resize(dist.size());
        for (int c = 0; c < dist.size(); ++c) {
            bandwidth[r][c] = 2 * 20 * 70 * dist[r][c] / 60;
        }
    }
    return bandwidth;
}


// write mps file with problem statement that will be used by glpsol
void write_mps(mss &to_abbrev) {
    // verts is the list of vertices full names
    // popul contains population numbers by vertex full name
    // index maps vertex full name to its index in verts
    // dist is the matrix of distances where 0 means there is no straight road
    vs verts;
    msi popul, index;
    vvi dist;
    read_graph(verts, popul, index, dist);

    // matrix of bandwidth of each edge
    auto bandwidth = count_bandwidth(dist);

    const int N = verts.size();

    // result file
    ofstream mps;
    mps.open(MPSFILE);

    // problem name
    mps << "NAME          PK_FLOWS\n";
    // describe matrix rows
    mps << "ROWS\n";
    // the first one is the target function with max flow value
    mps << " N\t_MAX_FLOW_\n";
    // rows of vertices constraints
    for (auto &v: verts) {
        mps << " E  _IN__" << to_abbrev[v] << "_\n";
        mps << " E  _OUT_" << to_abbrev[v] << "_\n";
    }

    // describe problem variables that are flow values at each edge
    mps << "COLUMNS\n";

    string negone = "_\t\t\t-1\n";
    string posone = "_\t\t\t1\n";

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] == 0) continue;
            
            // constraints on flow saving: input sum have to be equal to output sum at each vertex
            string edge = "\t" + to_abbrev[verts[r]] + '_' + to_abbrev[verts[c]] + "\t\t";
            mps << edge << "_MAX_FLOW" << posone;
            mps << edge << "_OUT_" << to_abbrev[verts[r]] << negone;
            mps << edge << "_IN__" << to_abbrev[verts[c]] << posone;
        }
    }

    for (int i = 0; i < N; ++i) {
        string edge = "\tJUN__" + to_abbrev[verts[i]] + "\t\t";
        mps << edge << "_IN__" << to_abbrev[verts[i]] << negone;
        mps << edge << "_OUT_" << to_abbrev[verts[i]] << posone;
    }

    // variables (ie edges flows) bounds: 0 at the bottom and maxflow at the top
    mps << "BOUNDS\n";
    string ub = " UP FLOW\t\t\t", lb = " LO FLOW\t\t\t";

    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] == 0) continue;

            string edge = to_abbrev[verts[r]] + '_' + to_abbrev[verts[c]] + "\t\t";
            mps << ub << edge << bandwidth[r][c] << endl;
            mps << lb << edge << 0 << endl;
        }
    }

    for (int i = 0; i < N; ++i) {
        string edge = "JUN__" + to_abbrev[verts[i]] + "\t\t";
        int jun = round(sigmd(popul[verts[i]]));
        mps << ub << edge << jun << endl;
        mps << lb << edge << 0 << endl;
    }

    mps << "ENDATA\n";
    mps.close();
}


// helper to ignore some lines in input file
void ignorelines(ifstream &input, int n = 1) {
    string tmpstr;
    for (int i = 0; i < n; ++i)
        getline(input, tmpstr);
}

// helper to ignore words in input file until the stop word is camed across
void ignorewhile(ifstream &input, string stop) {
    string tmpstr;
    while (input >> tmpstr)
        if (tmpstr == stop)
            break;
}


// parse edge of the following format 'FROM_TO' into to vertices
pss parse_edge(string &edge) {
    string from = edge.substr(0, 4), to = edge.substr(5, 4);
    return {from, to};
}


// parse glpsol solution
// specifically extract critical edges (having flow > 0)
void parse_output(mss &from_abbrev) {
    // the first one will be in format for graphviz, the second one in human-readable form
    ofstream flows_dot, flows_txt;
    flows_dot.open(DOTFILE);
    // declare directed graph in dot file
    flows_dot << "digraph {\n";
    flows_txt.open("flows.txt");

    // file with the problem solution that was generated by glpsol
    ifstream table_file;
    table_file.open(OUTPUTFILE);

    // firstly here goes table with row activities that we are not interesting in
    // ignore all the input until the word "Column" comes across
    // it will occure at the beginning of columns activities table (ie variables) 
    ignorewhile(table_file, "Column");
    // ignore table header
    ignorelines(table_file, 2);

    // table row is given in the following format: "No Column St Activity LB UB Marginal"
    // extract only Column, Activity, UB from each line
    string edge, tmpstr;
    int flow, bound;
    for (int no = 1; no <= 148; ++no) {
        // if the table suddenly ends
        // it becomes clear with the absense on No that is just integer
        table_file >> tmpstr;

        if (tmpstr != to_string(no)) break;

        // read line word by word using stringstream
        getline(table_file, tmpstr);
        stringstream ss(tmpstr);
        ss >> edge;
        ss >> tmpstr;
        ss >> flow;
        ss >> tmpstr;
        ss >> bound;

        // write extracted info to flows files
        auto [from, to] = parse_edge(edge);
        if (flow != 0) {
            flows_dot << '\t' << from << " -> " << to << " [label=\" " <<
                flow << "\",arrowsize=0.5,fontsize=12]\n";
            flows_txt << from_abbrev[from] << '-' << from_abbrev[to] << " & " << flow << " \\\\\n"; 
        }
    }
    flows_dot << '}';

    flows_dot.close();
    table_file.close();
}


int main(int argc, char **argv) {
    // maps from full vertices names to abbreviations and vice versa
    mss to_abbrev, from_abbrev;
    read_abbrev(to_abbrev, from_abbrev);

    // generate LP problem statement for glpsol
    write_mps(to_abbrev);

    // call glpsol to solve it
    system(("glpsol " + MPSFILE + " --max -o " + OUTPUTFILE).data());

    // parse critical edges from glpsol solution
    parse_output(from_abbrev);

    // call graphviz to draw graph of flows
    system(("dot -Tjpeg -Gdpi=300 -O " + DOTFILE).data());

    return 0;
}

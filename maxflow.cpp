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


// each vertex name is assosiated with its abbreviation consisting of exactly 4 letters
// they are listed in 'MODELPATH/abbreviations.txt" in following format "vertex_name abbreviation"
// to_abbrev maps vertex name to its abbreviation, from_abbrev - vice versa
void read_abbrev(map<string, string> &to_abbrev, map<string, string> &from_abbrev) {
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


// check whether vertex given by user is valid
// vertex is valid if its listed in abbreviations 
void approve_vert(string &vert, map<string, string> &to_abbrev) {
    // there must be only one name for it
    if (vert == "Артем") vert = "Артём";
    // if the name is unknown then decline it by assigning to empty string
    if (to_abbrev.count(vert) == 0)
        vert = "";
}


// read source and destination vertex given by user
pair<string, string> read_src_dst(int argc, char **argv, map<string, string> &to_abbrev) {
    // first try to read from 'src_dst.txt'
    string file_src, file_dst;
    // if the file is not present ignore it
    try {
        ifstream src_dst_file;
        src_dst_file.open("src_dst.txt");
        // source and destination are assumed to be listed one by one
        src_dst_file >> file_src >> file_dst;
        // check whether they are valid
        approve_vert(file_src, to_abbrev);
        approve_vert(file_dst, to_abbrev);
        src_dst_file.close();
    }
    catch (const exception &e) {}

    // try to read source and destination from command line arguments and check them
    auto arg_src = (argc >= 2 ? string(argv[1]) : "");
    approve_vert(arg_src, to_abbrev);
    auto arg_dst = (argc >= 3 ? string(argv[2]) : "");
    approve_vert(arg_dst, to_abbrev);

    // if finally there is no valid source or destination print message about it and exit with error
    if ((file_src == "" and arg_src == "") or (file_dst == "" and arg_dst == "")) {
        clog << "No source or destination was given, shutdown\n";
        clog << "Provide source and destination in \'" << MODELPATH << "src_dst.txt\' or via command line arguments\n";
        exit(1);
    }

    // command line args have higher priority
    return {(arg_src != "") ? arg_src : file_src, (arg_dst != "") ? arg_dst : file_dst};
}


// read all the vertices and edges of PK graph
// verts is the list of vertices full names
// popul contains population numbers by vertex full name
// index maps vertex full name to its index in verts
// dist is the matrix of distances where 0 means there is no straight road
void read_graph(vector<string> &verts, map<string, int> &popul, map<string, int> &index, vector<vector<int>> &dist) {
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
    sort(verts.begin(), verts.end(), [&](string &a, string &b) { return popul[a] > popul[b]; });

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


// write mps file with problem statement that will be used by glpsol
void write_mps(string &src, string &dst, map<string, string> &to_abbrev) {
    // verts is the list of vertices full names
    // popul contains population numbers by vertex full name
    // index maps vertex full name to its index in verts
    // dist is the matrix of distances where 0 means there is no straight road
    vector<string> verts;
    map<string, int> popul, index;
    vector<vector<int>> dist;
    read_graph(verts, popul, index, dist);

    const int N = verts.size();
    // source and destination indices in verts
    int src_idx = index[src], dst_idx = index[dst];

    // result file
    ofstream mps;
    mps.open(MPSFILE);

    // problem name
    mps << "NAME          PK_ROADS\n";
    // describe matrix rows
    mps << "ROWS\n";
    // the first one is the target function with max flow value
    mps << " N\tMAXFLOW\n";
    // rows of vertices constraints
    for (int i = 0; i < N; ++i) {
        if (i != src_idx and i != dst_idx)
            mps << " E  IO_" << to_abbrev[verts[i]] << '\n';
    }
    // the same but for source and destination
    mps << " E  SRC_DST\n";

    // describe problem variables that are flow values at each edge
    mps << "COLUMNS\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] > 0 and c != src_idx and r != dst_idx) {
                string var = "\t" + to_abbrev[verts[r]] + '_' + to_abbrev[verts[c]] + "\t\t";

                // if edge begins in source then it should be counted in target MAXFLOW variable
                if (r == src_idx){
                    mps << var << "MAXFLOW\t\t\t1\n";
                    if (c == dst_idx) continue;
                }
                
                // constraints on flow saving: input sum have to be equal to output sum at each vertex except src, dst
                // in case of src, dst the source output have to be equal to destination input
                if (r != src_idx and c != dst_idx){
                    mps << var << "IO_" << to_abbrev[verts[r]] << "\t\t\t-1\n";
                    mps << var << "IO_" << to_abbrev[verts[c]] << "\t\t\t1\n";
                } else if (r == src_idx) {
                    mps << var << "SRC_DST" << "\t\t\t-1\n";
                    mps << var << "IO_" << to_abbrev[verts[c]] << "\t\t\t1\n";
                } else if (c == dst_idx) {
                    mps << var << "IO_" << to_abbrev[verts[r]] << "\t\t\t-1\n";
                    mps << var << "SRC_DST" << "\t\t\t1\n";
                }
            }
        }
    }

    // variables (ie edges flows) bounds: 0 at the bottom and maxflow at the top
    mps << "BOUNDS\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            if (dist[r][c] > 0 and c != src_idx and r != dst_idx) {
                int cap = 2 * 20 * 70 * dist[r][c] / 60;
                mps << " UP FLOW\t\t\t" << to_abbrev[verts[r]] << '_' << to_abbrev[verts[c]] << "\t\t" << cap << '\n';
                mps << " LO FLOW\t\t\t" << to_abbrev[verts[r]] << '_' << to_abbrev[verts[c]] << "\t\t0\n";
            }
        }
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
pair<string,string> parse_edge(string &edge) {
    string from = edge.substr(0, 4), to = edge.substr(5, 4);
    return {from, to};
}


// parse glpsol solution
// specifically extract critical edges (having flow > 0)
void parse_output(map <string, string> &from_abbrev) {
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
    for (int no = 1; table_file >> tmpstr; ++no) {
        // if the table suddenly ends
        // it becomes clear with the absense on No that is just integer
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
            flows_dot << '\t' << from_abbrev[from] << " -> " << from_abbrev[to] << " [label=\" " <<
                flow << " / " << bound << "\",arrowsize=0.5,fontsize=8]\n";
            flows_txt << from_abbrev[from] << ' ' << from_abbrev[to] << ' ' << flow << ' ' << bound << '\n'; 
        }
    }
    flows_dot << '}';

    flows_dot.close();
    table_file.close();
}


int main(int argc, char **argv) {
    // maps from full vertices names to abbreviations and vice versa
    map<string, string> to_abbrev, from_abbrev;
    read_abbrev(to_abbrev, from_abbrev);

    // get source and destination by their names
    // it will exit with error if any of them isnt presented
    auto [src, dst] = read_src_dst(argc, argv, to_abbrev);
    // generate LP problem statement for glpsol
    write_mps(src, dst, to_abbrev);

    // call glpsol to solve it
    system(("glpsol " + MPSFILE + " --max --nopresol -o " + OUTPUTFILE).data());

    // parse critical edges from glpsol solution
    parse_output(from_abbrev);

    // call graphviz to draw graph of flows
    system(("dot -Tjpeg -Gdpi=300 -O " + DOTFILE).data());

    return 0;
}

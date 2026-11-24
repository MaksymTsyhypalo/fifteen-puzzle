#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>

using namespace std;

struct Board {
    int R, C;
    uint64_t packed = 0;
    std::vector<uint8_t> vec;
    int zero_pos = 0;

    bool is_packed() const {return R * C <= 16; }

    uint8_t get (int idx) const{
        if (is_packed()) return (packed >> (idx * 4)) & 0xFULL;
        return vec[idx];
    }

    void set(int idx, uint8_t val){
        if (is_packed()) {
            uint64_t mask = ~(0xFULL << (idx * 4));
            packed = (packed & mask) | (static_cast<uint64_t>(val) << (idx * 4));
        }
        else {
            vec[idx] = val;
        }
    }

    bool operator==(const Board& o) const {
        if (is_packed()) return packed == o.packed;
        return vec == o.vec;
    }
};

struct BoardHash {
    size_t operator()(const Board& b) const {
        if (b.is_packed()) {
            uint64_t x = b.packed;
            x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
            x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
            return x ^ (x >> 31);
        }
        size_t h = 0;
        for (uint8_t v : b.vec) h = h * 31 + v;
        return h;
    }
};

int comp_heur(const Board& b, int h_id){
    if (h_id == 0) return 0;

    int R = b.R, C = b.C;
    int manhattan = 0;

    for (int i = 0; i < R * C; ++i) {
        uint8_t val = b.get(i);
        if (val == 0) continue;
        int target_idx = val - 1;
        int r1 = i / C, c1 = i % C;
        int r2 = target_idx / C, c2 = target_idx % C;
        manhattan += abs(r1 - r2) + abs(c1 - c2);
    }

    if (h_id == 1) return manhattan;

    int conf_res = 0;

    for (int r = 0; r < R; ++r) {
        for (int c1 = 0; c1 < C; ++c1) {
            uint8_t v1 = b.get(r * C + c1);
            if (v1 == 0 || (v1 - 1) / C != r) continue;
            for (int c2 = c1 + 1; c2 < C; ++c2) {
                uint8_t v2 = b.get(r * C + c2);
                if (v2 == 0 || (v2 - 1) / C != r) continue;
                if (v1 > v2) conf_res += 2;
            }
        }
    }

    for (int c = 0; c < C; ++c) {
        for (int r1 = 0; r1 < R; ++r1) {
            uint8_t v1 = b.get(r1 * C + c);
            if (v1 == 0 || (v1 - 1) % C != c) continue;
            for (int r2 = r1 + 1; r2 < R; ++r2) {
                uint8_t v2 = b.get(r2 * C + c);
                if (v2 == 0 || (v2 - 1) % C != c) continue;
                if (v1 > v2) conf_res += 2;
            }
        }
    }
    
    return manhattan + conf_res;
}

bool apply_move(const Board& curr, char move, Board& next){
    int r = curr.zero_pos / curr.C;
    int c = curr.zero_pos % curr.C;
    int nr = r, nc = c;

    if (move == 'L') nc += 1;
    else if (move == 'R') nc -= 1;
    else if (move == 'U') nr += 1;
    else if (move == 'D') nr -= 1;
    else return false;

    if (nr < 0 || nr >= curr.R || nc < 0 || nc >= curr.C) return false;
    next = curr;
    int new_zero = nr * curr.C + nc;
    uint8_t tile = curr.get(new_zero);
    next.set(curr.zero_pos, tile);
    next.set(new_zero, 0);
    next.zero_pos = new_zero;
    return true;
}

char opp_move(char m){
    if (m == 'L') return 'R';
    if (m == 'R') return 'L';
    if (m == 'U') return 'D';
    if (m == 'D') return 'U';
    return 0;
}

bool solvable(const Board& b){
    int inv = 0;
    int N = b.R * b.C;
    for (int i = 0; i<N; ++i){
        uint8_t vi = b.get(i);
        if(vi == 0) continue;
        for(int j = i + 1; j < N; ++j){
            uint8_t vj = b.get(j);
            if(vj != 0 && vi > vj) inv++;
        }
    }
    if (b.C % 2 == 1) {
        return (inv % 2 == 0);
    } else {
        int blank_row_from_bottom = b.R - (b.zero_pos / b.C);
        return ((inv + blank_row_from_bottom) % 2 == 1);
    }
}

struct Metrics {
    long long visited_states = 0;
    long long processed_states = 0;
    int max_recursion_depth = 0;
    double elapsed_ms = 0.0;
};

std::mt19937 rng(42);

std::string get_move_order(const std::string& order_spec){
    if (!order_spec.empty() && order_spec[0] == 'R' && order_spec.size() == 1) {
        std::string dirs = "LRUD";
        shuffle(dirs.begin(), dirs.end(), rng);
        return dirs;
    }

     if (!order_spec.empty() && order_spec[0] == 'R') {
        std::string dirs = "LRUD";
        shuffle(dirs.begin(), dirs.end(), rng);
        return dirs;
    }
    return order_spec;
}

std::string solve_bfs(const Board& start, const Board& goal, const std::string& order, Metrics& m){
    if (start == goal) return "";
    struct Node { Board b; std::string path; };
    std::queue<Node> q;
    std::unordered_set<Board, BoardHash> visited;

    q.push({start, ""});
    visited.insert(start);
    m.visited_states = 1;

    while (!q.empty()){
        Node curr = std::move(q.front());
        q.pop();
        m.processed_states++;

        std::string dirs = get_move_order(order);
        for (char d : dirs){
            if (!curr.path.empty() && d == opp_move(curr.path.back())) continue;
            Board next;
            if(apply_move(curr.b, d, next)){
                if (next == goal){
                    m.max_recursion_depth = curr.path.size() + 1;
                    return curr.path + d;
                }
                if (visited.insert(next).second){
                    m.visited_states++;
                    int depth = curr.path.size() + 1;
                    if (depth > m.max_recursion_depth) m.max_recursion_depth = depth;
                    q.push({next, curr.path + d});
                }
            }
        }
    }
    return "UNSOLVED";
}

std::string solve_dfs(const Board& start, const Board& goal, const std::string& order, int max_depth, Metrics& m) {
struct Frame { Board b; std::string path; };
    std::vector<Frame> st;
    std::unordered_map<Board, int, BoardHash> best_depth;

    st.push_back({start, ""});
    best_depth[start] = 0;
    m.visited_states = 1;

    while (!st.empty()) {
        Frame curr = std::move(st.back());
        st.pop_back();
        m.processed_states++;
        int d_len = curr.path.size();
        if (d_len > m.max_recursion_depth) m.max_recursion_depth = d_len;

        if (curr.b == goal) return curr.path;
        if (d_len >= max_depth) continue;

        std::string dirs = get_move_order(order);
        for (int i = (int)dirs.size() - 1; i >= 0; --i) {
            char d = dirs[i];
            if (!curr.path.empty() && d == opp_move(curr.path.back())) continue;
            Board next;
            if (apply_move(curr.b, d, next)) {
                int next_d = d_len + 1;
                auto it = best_depth.find(next);
                if (it == best_depth.end() || next_d < it->second) {
                    best_depth[next] = next_d;
                    m.visited_states++;
                    st.push_back({next, curr.path + d});
                }
            }
        }
    }
    return "UNSOLVED";
}

bool  dfs_recur(const Board& curr, const Board& goal, int limit, std::string& path, const std::string& order, std::unordered_map<Board, int, BoardHash>& visited, Metrics& m){
    m.processed_states++;
    int depth = path.size();
    if (depth > m.max_recursion_depth) m.max_recursion_depth = depth;
    if (curr == goal) return true;
    if (depth >= limit) return false;

    visited[curr] = depth;
    std::string dirs = get_move_order(order);

    for (char d : dirs){
        if (!path.empty() && d == opp_move(path.back())) continue;
        Board next;
        if (apply_move(curr, d, next)) {
            auto it = visited.find(next);
            if (it == visited.end() || depth + 1 < it-> second){
                m.visited_states++;
                path.push_back(d);
                if (dfs_recur(next, goal, limit, path, order, visited, m)) return true;
                path.pop_back();
            }
        }
    }
    return false;
}

std::string solve_idfs(const Board& start, const Board& goal, const std::string& order, int max_limit, Metrics& m){
    for (int limit = 0; limit <= max_limit; ++limit){
        string path = "";
        unordered_map<Board, int, BoardHash> visited;
        m.visited_states++;
        if (dfs_recur(start, goal, limit, path, order, visited, m)) {
            return path;
        }
    }
    return "UNSOLVED";
}

string solve_informed(const Board& start, const Board& goal, int h_id, bool is_astar, Metrics& m){
    struct PQNode {
        Board b;
        int g;
        int f;
        string path;
        bool operator>(const PQNode& o) const {
            if(f != o.f) return f > o.f;
            return g < o.g;
        }
    };

    priority_queue<PQNode, vector<PQNode>, greater<PQNode>> pq;
    unordered_map<Board, int, BoardHash> best_g;

    int h0 = comp_heur(start, h_id);
    pq.push({start, 0, h0, ""});
    best_g[start] = 0;
    m.visited_states = 1;

    while (!pq.empty()){
        PQNode curr = std::move(const_cast<PQNode&>(pq.top()));
        pq.pop();

        if (is_astar && curr.g > best_g[curr.b]) continue;
        m.processed_states++;
        if ((int)curr.path.size() > m.max_recursion_depth) m.max_recursion_depth = curr.path.size();

        if (curr.b == goal) return curr.path;

        for (char d : {'L', 'R', 'U', 'D'}) {
            if (!curr.path.empty() && d == opp_move(curr.path.back())) continue;
            Board next;
            if (apply_move(curr.b, d, next)) {
                int ng = curr.g + 1;
                auto it = best_g.find(next);
                if (it == best_g.end() || (is_astar && ng < it->second)) {
                    best_g[next] = ng;
                    m.visited_states++;
                    int h = comp_heur(next, h_id);
                    int f = is_astar ? (ng + h) : h;
                    pq.push({next, ng, f, curr.path + d});
                }
            }
        }
    }
    return "UNSOLVED";
}

string solve_sma(const Board& start, const Board& goal, int h_id, size_t max_nodes, Metrics& m){
    struct SMANode {
        Board b;
        int g;
        int f;
        string path;
        uint64_t id;
        bool operator<(const SMANode& o) const {
            if (f != o.f) return f < o.f;
            if (g != o.g) return g > o.g;
            return id < o.id;
        }
    };

    set<SMANode> open_set;
    unordered_map<Board, set<SMANode>::iterator, BoardHash> lookup;
    uint64_t next_id = 0;

    int h0 = comp_heur(start, h_id);
    auto res = open_set.insert({start, 0, h0, "", next_id++});
    lookup[start] = res.first;
    m.visited_states = 1;

    while (!open_set.empty()) {
        SMANode curr = *open_set.begin();
        open_set.erase(open_set.begin());
        lookup.erase(curr.b);

        m.processed_states++;
        if ((int)curr.path.size() > m.max_recursion_depth) m.max_recursion_depth = curr.path.size();

        if (curr.b == goal) return curr.path;

        for (char d : {'L', 'R', 'U', 'D'}) {
            if (!curr.path.empty() && d == opp_move(curr.path.back())) continue;
            Board next;
            if (apply_move(curr.b, d, next)) {
                int ng = curr.g + 1;
                int nf = max(curr.f, ng + comp_heur(next, h_id));

                auto it = lookup.find(next);
                if (it != lookup.end()) {
                    if (ng < it->second->g) {
                        open_set.erase(it->second);
                        lookup.erase(it);
                    } else {
                        continue;
                    }
                }

                if(open_set.size() >= max_nodes){
                    auto worst_it = prev(open_set.end());
                    lookup.erase(worst_it->b);
                    open_set.erase(worst_it);
                }

                auto ins = open_set.insert({next, ng, nf, curr.path + d, next_id++});
                lookup[next] = ins.first;
                m.visited_states++;
            }
        }
    }
    return "UNSOLVED";
}

int main(int argc, char* argv[]){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string strategy = "";
    string param = "";
    bool log_metrics = false;
    int dfs_limit = 80;
    size_t sma_limit = 50000;

    for (int i = 1; i < argc; ++i){
        string arg = argv[i];
        if ((arg == "-b" || arg == "--bfs") && i + 1 < argc) { strategy = "bfs"; param = argv[++i]; }
        else if ((arg == "-d" || arg == "--dfs") && i + 1 < argc) { strategy = "dfs"; param = argv[++i]; }
        else if ((arg == "-i" || arg == "--idfs") && i + 1 < argc) { strategy = "idfs"; param = argv[++i]; }
        else if ((arg == "-h" || arg == "--bf") && i + 1 < argc) { strategy = "bf"; param = argv[++i]; }
        else if ((arg == "-a" || arg == "--astar") && i + 1 < argc) { strategy = "astar"; param = argv[++i]; }
        else if ((arg == "-s" || arg == "--sma") && i + 1 < argc) { strategy = "sma"; param = argv[++i]; }
        else if (arg == "--metrics") { log_metrics = true; }
        else if (arg == "--max-depth" && i + 1 < argc) { dfs_limit = stoi(argv[++i]); }
        else if (arg == "--sma-mem" && i + 1 < argc) { sma_limit = stoull(argv[++i]); }
    }

    int R, C;
    if (!(cin>>R>>C)) return 1;

    Board start{R, C, 0, vector<uint8_t>(R * C, 0), 0};
    Board goal{R, C, 0, vector<uint8_t>(R * C, 0), R * C - 1};

    for (int i = 0; i < R * C; ++i) {
        int val;
        cin >> val;
        start.set(i, static_cast<uint8_t>(val));
        if (val == 0) start.zero_pos = i;
        goal.set(i, (i == R * C - 1) ? 0 : static_cast<uint8_t>(i + 1));
    }

    Metrics metrics;
    auto t0 = chrono::high_resolution_clock::now();

    string solution = "UNSOLVED";
    if (!solvable(start)) {
        solution = "UNSOLVED";
    } else if (strategy == "bfs") {
        solution = solve_bfs(start, goal, param, metrics);
    } else if (strategy == "dfs") {
        solution = solve_dfs(start, goal, param, dfs_limit, metrics);
    } else if (strategy == "idfs") {
        solution = solve_idfs(start, goal, param, dfs_limit, metrics);
    } else if (strategy == "bf") {
        solution = solve_informed(start, goal, stoi(param), false, metrics);
    } else if (strategy == "astar") {
        solution = solve_informed(start, goal, stoi(param), true, metrics);
    } else if (strategy == "sma") {
        solution = solve_sma(start, goal, stoi(param), sma_limit, metrics);
    }

    auto t1 = chrono::high_resolution_clock::now();
    metrics.elapsed_ms = chrono::duration<double, milli>(t1 - t0).count();

    if (solution == "UNSOLVED"){
        cout << -1 <<"\n\n";
    }
    else{
        cout << solution.size()<<"\n" << solution << endl;
    }

    if (log_metrics) {
        cerr << "METRICS|"
             << "len=" << (solution == "UNSOLVED" ? -1 : (int)solution.size()) << "|"
             << "visited=" << metrics.visited_states << "|"
             << "processed=" << metrics.processed_states << "|"
             << "max_depth=" << metrics.max_recursion_depth << "|"
             << "time_ms=" << metrics.elapsed_ms << "\n";
    }

    return 0;
}
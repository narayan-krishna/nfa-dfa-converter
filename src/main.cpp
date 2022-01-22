#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

class NFA {
    public:
        vector<int> states;
        vector<char> alphabet;
        int start_state;
        vector<int> accept_states; //these needs to separate based on commas
        unordered_map<int, unordered_map<char, vector<int>>> transitions;

        NFA(const string filename);
        const void print_out();

    private:
        inline void parse_states(const string line);
        inline void parse_alphabet(const string line);
        inline void get_start_state(const string line);
        inline void parse_valid_accept_states(const string line);
        inline void parse_transition(const string line);
};

NFA::NFA(const string filename) {
    ifstream parse{ filename };
    string current; int line_number = 0;
    while(getline(parse, current)) {
        if(line_number < 4) {
            if(line_number == 0) parse_states(current);
            if(line_number == 1) parse_alphabet(current);
            if(line_number == 2) get_start_state(current);
            if(line_number == 3) parse_valid_accept_states(current);
        } else {
            parse_transition(current);
        }
        line_number++;
    }
}

inline void NFA::parse_states(const string line) {
    for(int i = 1; i < line.length(); i += 4) {
        states.push_back(line[i] - '0');
    }
}

inline void NFA::parse_alphabet(const string line) {
    for(int i = 0; i < line.length(); i += 2) {
        alphabet.push_back(line[i]);
    }
}

inline void NFA::get_start_state(const string line) {
    // cout << line << endl;
    start_state = line[1] - '0';
}

inline void NFA::parse_valid_accept_states(const string line) {
    for(int i = 1; i < line.length(); i += 4) {
        accept_states.push_back(line[i] - '0');
    }
}

inline void NFA::parse_transition(const string line) {
    int init_state = line[1] - '0';
    // cout << line << endl;

    char symbol;
    if (line[5] == 'E') {
        if (line[6] == 'P' && line[7] == 'S') {
            // cout << "found EPS" << endl;
            symbol = '-';
        } 
    } else {
        symbol = line[5];
    }

    // cout << "symbol: " << symbol << endl;

    int end_state = line[line.length() - 2] - '0';
    
    // cout << init_state << ", " << symbol << ", " << end_state << endl;

    auto iter = transitions.find(init_state);
    if (iter != end(transitions)) { //contains this state
        // iter->second.push_back({symbol, end_state});
        auto iter2 = (iter->second).find(symbol);
        if (iter2 != end(iter->second)) {
            iter2->second.push_back(end_state);
        } else {
            vector<int> states; states.push_back(end_state);
            iter->second.insert({symbol, states});
        }
    } else {
        // vector<pair<char, int>> symbol_to_end;
        // symbol_to_end.push_back({symbol, end_state});
        // transitions.insert({init_state, symbol_to_end});
        vector<int> states; states.push_back(end_state);
        unordered_map<char, vector<int>> symbol_map;
        symbol_map.insert({symbol, states});
        transitions.insert({init_state, symbol_map});
    }
}

const void NFA::print_out() {
    for(auto i : states) {
        cout << i << " ";
    } cout << endl;

    for(auto i : alphabet) {
        cout << i << " ";
    } cout << endl;

    cout << start_state << endl;

    for(auto i : accept_states) {
        cout << i << " ";
    } cout << endl;

    for(auto i : transitions) {
        cout << i.first << ": " << endl;
        for (auto j : i.second) {
            cout << "symbol: " << j.first << ": ";
            for (auto k : j.second) {
                cout << k << " ";
            } cout << endl;
        }
    }
}

class DFA {
    typedef unordered_set<int> dfa_state;

    private:
        vector<dfa_state> states;
        vector<char> alphabet;
        int start_state;
        vector<dfa_state> accept_states;
        vector<pair<vector<int>, unordered_map<char, dfa_state>>> transitions;
        vector<vector<int>> powerset;

        void generate_powerset(const vector<int>& nums);
        void powerset_dfs(const vector<int>& nums, vector<int> &curr_subset, 
                       vector<vector<int>>& subsets, int pos, int contains_start);
        void generate_transitions_from_powerset(const NFA &nfa);
        void epsilon_check(int state, dfa_state& states, const NFA &nfa, int init);

    public:
        DFA(const NFA &nfa);
        const void print_to_file(string file_name);
};

DFA::DFA(const NFA &nfa) {
    start_state = nfa.start_state;
    alphabet = nfa.alphabet; 
    generate_powerset(nfa.states);
    generate_transitions_from_powerset(nfa);
}

void DFA::powerset_dfs(const vector<int>& nums, vector<int> &curr_subset, 
                       vector<vector<int>>& subsets, int pos, int contains_start) {
    if (contains_start) {
        dfa_state accept(curr_subset.begin(), curr_subset.end());
        accept_states.push_back(accept);
    }

    if (pos >= nums.size()) {
        vector<int> copy(curr_subset);
        subsets.push_back(copy); 
        return;
    }
    
    vector<int> included(curr_subset);
    included.push_back(nums[pos]);
    // int contains = 0;
    // if (nums[pos] == start_state) contains = 1;
    powerset_dfs(nums, included, subsets, pos + 1, 
                 nums[pos] == start_state ? 1 : contains_start);
        
    vector<int> non_included(curr_subset);
    powerset_dfs(nums, non_included, subsets, pos + 1, 0);
} 
    
void DFA::generate_powerset(const vector<int>& nums) {
    vector<int> init_subset;
    powerset_dfs(nums, init_subset, powerset, 0, 0);
}

void DFA::generate_transitions_from_powerset(const NFA &nfa) {
    for (auto set : powerset) {
        vector<int> copied_set(set);
        unordered_map<char, dfa_state> final; // {}, x => {}
        int original_set_size = set.size(); 
        unordered_set<int> states_checked;

        for (int i = 0; i < copied_set.size(); i++) {
            if (states_checked.find(copied_set[i]) != end(states_checked)) continue;
            auto nfa_transition_state = nfa.transitions.find(copied_set[i]); //find each state within transition map of nfa

            //error control
            if (nfa_transition_state == end(nfa.transitions)) {
                cerr << "state not found in nfa" << endl;
                exit (EXIT_FAILURE);
            } 
            //error control
            //parse the second part of transaction mapping (symbol to next state)
            for(auto symbol_mapping : nfa_transition_state->second) { //every symbol mapping
                char character_mapped = symbol_mapping.first;
                if (character_mapped == '-') {
                    for(auto state : symbol_mapping.second) {
                        copied_set.push_back(state);
                    }
                } else {
                    auto symbol_check = final.find(character_mapped); //is the symbol from nfa in dfa
                    if(symbol_check == end(final)) { //if we've encountered before, push the state to corresponding vec
                        dfa_state state;
                        final.insert({character_mapped, state});
                        symbol_check = final.find(character_mapped); //so we can use this iterator for later
                    }

                    for(auto state : symbol_mapping.second) { //for states associated with char
                        symbol_check->second.insert(state); //push back states to their corresponding letter
                        epsilon_check(state, symbol_check->second, nfa, 1);
                    }
                }
            } 

            states_checked.insert(copied_set[i]);
        }

        // unordered_set<int> removed_dups(copied_set.begin(), copied_set.end());
        transitions.push_back({set, final});
    }
}

void DFA::epsilon_check(int state, dfa_state& states, const NFA &nfa, int init) {
    if (states.find(state) != states.end() && init == 0) return;

    auto transition_iter = nfa.transitions.find(state);
    auto epsilon_associations = transition_iter->second.find('-');

    if (epsilon_associations == end(transition_iter->second)) return;

    for(auto i : epsilon_associations->second) {
        states.insert(i);
        epsilon_check(i, states, nfa, 0);
    }
}

const void DFA::print_to_file(string file_name) {
    ofstream outfile;
    outfile.open (file_name + ".dfa");

    //list of states
    string state_list = "";
    for (auto state : powerset) {
        if (state.size() > 0) {
            string power_rep = "{";
            for (auto members : state) {
                // cout << members << endl;
                if (state.size() > 0) { 
                    power_rep += '0' + members;
                    power_rep += ',';
                } 
            }
            power_rep[power_rep.size() - 1] = '}';
            state_list += power_rep + " ";
        }
    }
    outfile << state_list + "{EM}" << endl;
    

    //list of symbols
    for (auto symbol : alphabet) {
        outfile << symbol << '\t';
    } outfile << endl;

    //start state
    outfile << "{" << start_state << + "}" << endl;

    //valid accept states
    string accept_state_list = "";
    for (auto states : accept_states) {
        if (states.size() > 0) {
            string power_rep = "{";
            for (int i : states) {
                // cout << members << endl;
                power_rep += '0' + i;
                power_rep += ',';
            }
            power_rep[power_rep.size() - 1] = '}';
            accept_state_list += power_rep + " ";
        }
    }
    outfile << accept_state_list << endl;

    // transition function
    for (auto transition : transitions) {
        string transition_rep = "{";
        for (auto states : transition.first) {
            transition_rep += '0' + states;
            transition_rep += ',';
        }
        transition_rep[transition_rep.size() - 1] = '}';
        transition_rep += ", ";
        for (auto map : transition.second) {
            string final_rep = transition_rep;
            if (map.first == '-') {
                final_rep += "EPS";
            } else {
                final_rep += map.first;
            }

            final_rep += " = {";
            for (auto state : map.second) {
                final_rep += '0' + state;
                final_rep += ',';
            }
            final_rep[final_rep.size() - 1] = '}';
            outfile << final_rep << " ";
            outfile << endl;
        }
    }

    outfile.close();
}

int main (int argc, char **argv) {

    if (argc != 2) {
        cerr << "requires file name!" << endl;
        exit(EXIT_FAILURE);
    }

    // set_test();

    NFA my_NFA = NFA(argv[1]);
    DFA my_DFA = DFA(my_NFA);
    my_DFA.print_to_file("first_dfa");

    return 0;
}

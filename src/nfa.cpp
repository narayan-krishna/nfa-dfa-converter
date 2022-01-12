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
        vector<int> accept_states;
        unordered_map<int, unordered_map<char, vector<int>>> transitions;

        NFA(const string filename);
        void print_out();

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

void NFA::print_out() {
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

typedef unordered_set<int> dfa_state;

class DFA {
    private:
        vector<dfa_state> states;
        vector<char> alphabet;
        vector<int> start_state;
        vector<dfa_state> accept_states;
        vector<pair<vector<int>, unordered_map<char, dfa_state>>> transitions;
        vector<vector<int>> powerset;

        void generate_powerset(const vector<int>& states, 
                               vector<vector<int>>& powerset);
        void generate_transitions_from_powerset(const NFA &nfa);
        void epsilon_check(int state, dfa_state& states, const NFA &nfa);

    public:
        DFA(const NFA &nfa);
};

DFA::DFA(const NFA &nfa) {
    start_state = vector<int>{nfa.start_state};
    alphabet = nfa.alphabet; 
    generate_powerset(nfa.states, powerset);
    generate_transitions_from_powerset(nfa);
}

void DFA::generate_powerset(const vector<int>& states, vector<vector<int>>& powerset) {
    for(int i = 0; i < states.size(); i++) {
        vector<int> single_set;
        for(int j = i; j < states.size(); j ++) {
            single_set.push_back(states[j]);
            // print_vec(single_set); 
            vector<int> copied(single_set);
            powerset.push_back(copied);
        }
    }

    for(auto vec : powerset) {
        for(int i : vec) {
            cout << i << " ";
        } cout << endl;
    }
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
                        epsilon_check(state, symbol_check->second, nfa);
                        // cout << state << endl;
                        if (i >= original_set_size) { //was this found through an epsilon closure?
                            symbol_check->second.insert(copied_set[i]);
                        }
                    }
                }
            } 

            states_checked.insert(copied_set[i]);
        }

        transitions.push_back({set, final});
        cout << "\n(";
        for (auto i : set) {
            cout << " " << i;
        } cout << " )" << endl;

        for(auto i : final) {
            cout << i.first << "=>";
            for(auto j : i.second) {
                cout << j << ",";
            } cout << endl;
        }
    }
}

void DFA::epsilon_check(int state, dfa_state& states, const NFA &nfa) {
    auto transition_iter = nfa.transitions.find(state);
    auto epsilon_associations = transition_iter->second.find('-');

    if (epsilon_associations == end(transition_iter->second)) return;

    for(auto i : epsilon_associations->second) {
        states.insert(i);
    }
}

int main (int argc, char **argv) {

    if (argc != 2) {
        cerr << "requires file name!" << endl;
        exit(EXIT_FAILURE);
    }

    // set_test();

    NFA my_NFA = NFA(argv[1]);
    my_NFA.print_out();
    DFA my_DFA = DFA(my_NFA);

    return 0;
}

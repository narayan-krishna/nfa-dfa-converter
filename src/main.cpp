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
        int nfa_start_state;
        dfa_state start_state;
        vector<dfa_state> accept_states;
        vector<pair<dfa_state, unordered_map<char, dfa_state>>> transitions;
        vector<dfa_state> powerset;

        void epsilon_check(int state, dfa_state& states, const NFA &nfa, int init);
        void get_start_state(const NFA &nfa);
        void generate_transitions_dynamic(const NFA &nfa);
        void generate_transitions (dfa_state process_state, vector<dfa_state> explored_states,
                                   const NFA &nfa);

    public:
        DFA(const NFA &nfa);
        const void print_to_file(string file_name);
};

DFA::DFA(const NFA &nfa) {
    get_start_state(nfa);
    alphabet = nfa.alphabet; 
    nfa_start_state = nfa.start_state;
    generate_transitions_dynamic(nfa);
}

void DFA::get_start_state(const NFA &nfa) {
    start_state.insert(nfa.start_state);
    epsilon_check(nfa.start_state, start_state, nfa, 1);
}

void DFA::generate_transitions_dynamic (const NFA &nfa) {
    vector<dfa_state> explored_states;
    generate_transitions(start_state, explored_states, nfa);
}

void DFA::generate_transitions (dfa_state process_state, vector<dfa_state> explored_states, 
                                const NFA &nfa) {
    for (auto state : explored_states) {
        if (state == process_state) {
            return;
        } 
    }

    vector<int> dfa_state_vector(process_state.begin(), process_state.end());
    unordered_map<char, dfa_state> process_state_mappings;
    unordered_set<int> states_checked;

    //equip map with all possible mappings for each character
    for (char alpha : alphabet) {
        dfa_state state;
        process_state_mappings.insert({alpha, state});
    }

    //check for epsilon states on each of the states in vector, add
    //epsilon states back on
    for (int i = 0; i < dfa_state_vector.size(); i ++) {
        auto nfa_iter = nfa.transitions.find(dfa_state_vector[i]);
        auto char_iter = nfa_iter->second.find('-');
        if (char_iter != nfa_iter->second.end()) {
            for(auto state : char_iter->second) {
                dfa_state_vector.push_back(state);
            }
        }
    }

    //go through the character mappings for each state
    //add all their corresponding states to map, espilon check them
    for (int i = 0; i < dfa_state_vector.size(); i ++) {

        if (states_checked.find(dfa_state_vector[i]) != end(states_checked)) continue;
        auto nfa_iter = nfa.transitions.find(dfa_state_vector[i]);

        for (char alpha : alphabet) {
            auto char_iter = nfa_iter->second.find(alpha);
            auto mapping_check = process_state_mappings.find(alpha);

            if (char_iter != nfa_iter->second.end()) {
                for(auto state : char_iter->second) { //for states associated with char
                    mapping_check->second.insert(state); //push back states to their corresponding letter
                    epsilon_check(state, mapping_check->second, nfa, 1);
                }
            }
        }

        states_checked.insert(dfa_state_vector[i]);
    } 

    dfa_state set(dfa_state_vector.begin(), dfa_state_vector.end());
    transitions.push_back({set, process_state_mappings});
    explored_states.push_back(set);
    states.push_back(set);

    if (set.find(nfa_start_state) != set.end()) accept_states.push_back(set);

    for (auto mapping : process_state_mappings) {
        generate_transitions(mapping.second, explored_states, nfa);
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
    for (auto state : states) {
        if (state.size() > 0) {
            string power_rep = "{";
            for (auto members : state) {
                power_rep += '0' + members;
                power_rep += ',';
            }
            power_rep[power_rep.size() - 1] = '}';
            state_list += power_rep + " ";
        } else {
            state_list += "{EM} ";
        }
    }
    outfile << state_list << endl;
    

    //list of symbols
    for (auto symbol : alphabet) {
        outfile << symbol << '\t';
    } outfile << endl;

    //start state
    string start_state_string = "{";
    for (auto states : start_state) {
        start_state_string += '0' + states;
        start_state_string += ',';
    }

    start_state_string[start_state_string.size() - 1] = '}';

    outfile << start_state_string << endl;

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
        if (transition.first.size() > 0) {
            for (auto states : transition.first) {
                transition_rep += '0' + states;
                transition_rep += ',';
            }
            transition_rep[transition_rep.size() - 1] = '}';
        } else {
            transition_rep += "EM";
            transition_rep += '}';
        }
        transition_rep += ", ";
        for (auto map : transition.second) {
            string final_rep = transition_rep;
            final_rep += map.first;

            final_rep += " = {";
            if (map.second.size() > 0) {
                for (auto state : map.second) {
                    final_rep += '0' + state;
                    final_rep += ',';
                }
                final_rep[final_rep.size() - 1] = '}';
            } else {
                final_rep += "EM";
                final_rep += '}';
            }
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

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

//nfa class (5-tuple) ---------------------------------------
class NFA {
    public:
        //member variables are representative of elements of 5-tuple
        //5-tuple (nfa states, alphabet, start state, accept states, transitions)
        vector<int> states;
        vector<char> alphabet;
        int start_state;
        vector<int> accept_states; //these needs to separate based on commas
        unordered_map<int, unordered_map<char, vector<int>>> transitions;

        //create an NFA from a file
        NFA(const string filename);
        //print out NFA info (mainly for testing)
        void print_out() const;

    private:
        //inline functions to process a file into a dfa
        inline void parse_states(const string line);
        inline void parse_alphabet(const string line);
        inline void get_start_state(const string line);
        inline void parse_valid_accept_states(const string line);
        inline void parse_transition(const string line);
};

//given an nfa file, parse for nfa 5-tuple
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

//check for epsilons in the line
inline void NFA::parse_transition(const string line) {
    int init_state = line[1] - '0';
    // cout << line << endl;

    char symbol;
    if (line[5] == 'E') {
        if (line[6] == 'P' && line[7] == 'S') {
            symbol = '-';
        } 
    } else {
        symbol = line[5];
    }

    int end_state = line[line.length() - 2] - '0';

    auto iter = transitions.find(init_state);
    if (iter != end(transitions)) { //contains this state
        auto iter2 = (iter->second).find(symbol);
        if (iter2 != end(iter->second)) {
            iter2->second.push_back(end_state);
        } else {
            vector<int> states; states.push_back(end_state);
            iter->second.insert({symbol, states});
        }
    } else {
        vector<int> states; states.push_back(end_state);
        unordered_map<char, vector<int>> symbol_map;
        symbol_map.insert({symbol, states});
        transitions.insert({init_state, symbol_map});
    }
}

//print out nfa for testing
void NFA::print_out() const {
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

//dfa class (5-tuple) ---------------------------------------
class DFA {
    //all dfa_states are essentially sets. this helps avoid duplicates, 
    //and now we can find single states in constant time
    //comparing sets can also be down using the == operator
    typedef unordered_set<int> dfa_state;

    private:
        //the 5-tuple, including the nfa_start_state so we can call on it later
        vector<dfa_state> states;
        vector<char> alphabet;
        int nfa_start_state;
        dfa_state start_state;
        vector<dfa_state> accept_states;
        vector<pair<dfa_state, unordered_map<char, dfa_state>>> transitions;

        //recursively epsilon a certain state
        void epsilon_check(int state, dfa_state& states, 
                           const NFA &nfa, int init);
        //acquire dfa start state (epsilon check nfa start state)
        void get_start_state(const NFA &nfa);
        //starts recursive transition generator
        void generate_transitions_dynamic(const NFA &nfa);
        //recursive helper function
        void generate_transitions (dfa_state process_state, 
                                   vector<dfa_state>& explored_states,
                                   const NFA &nfa);
        
        //functions returning strings for printing
        string string_states() const;
        inline string string_alphabet() const;
        inline string string_start_states() const;
        string string_accept_states() const;
        vector<string> string_transitions_vec() const;

    public:
        DFA(const NFA &nfa);
        void print_to_file(string file_name) const;
};

//create the dfa -- based around the 5-tuple. the states are created along
//with transitions
DFA::DFA(const NFA &nfa) {
    get_start_state(nfa);
    alphabet = nfa.alphabet; 
    nfa_start_state = nfa.start_state;
    generate_transitions_dynamic(nfa);
}

//the start the state is the nfa start state, with epsilon checking
void DFA::get_start_state(const NFA &nfa) {
    start_state.insert(nfa.start_state);
    epsilon_check(nfa.start_state, start_state, nfa, 1);
}

//starter function for generating transitions recursively
void DFA::generate_transitions_dynamic (const NFA &nfa) {
    vector<dfa_state> explored_states;
    generate_transitions(start_state, explored_states, nfa);
}

//generate transitions for a dfa state, and then process the created end states
//(recursive)
void DFA::generate_transitions (dfa_state process_state, vector<dfa_state>& explored_states, 
                                const NFA &nfa) {
    //base case. make sure we haven't already processed this state
    for (auto state : explored_states) {
        if (state == process_state) {
            return;
        } 
    }

    unordered_map<char, dfa_state> process_state_mappings;
    unordered_set<int> states_checked; //technically the same data type as
                                       //dfa state but not literally a dfa state 

    //equip map with all possible mappings for each character
    for (char alpha : alphabet) {
        dfa_state state;
        process_state_mappings.insert({alpha, state});
    }

    //move everything into a vector. this way we can operate on the vector
    //while changing it. 
    vector<int> dfa_state_vector(process_state.begin(), process_state.end());

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
        //since we aren't using a set for dfa_states in this case, 
        //we need to be careful of duplicates
        states_checked.insert(dfa_state_vector[i]);
    } 

    //copy everything back into a set to get rid of duplicates
    dfa_state set(dfa_state_vector.begin(), dfa_state_vector.end());

    transitions.push_back({set, process_state_mappings}); //push our transition
    explored_states.push_back(set); //push the states that we've checked
    states.push_back(set); //push the states to the overall dfa state list

    //this checks if the set contains the original start state
    //meeting this condition would mean that the state is an accept state
    if (set.find(nfa_start_state) != set.end()) accept_states.push_back(set);

    //for each of the states at the end of our mappings, 
    //generate transitions for them
    for (auto mapping : process_state_mappings) {
        generate_transitions(mapping.second, explored_states, nfa);
    }
}

//recursively epsilon check a single state
void DFA::epsilon_check(int state, dfa_state& states, const NFA &nfa, int init) {
    //base case. if the state has already been processed/added to states
    if (states.find(state) != states.end() && init == 0) return;

    //find the state in the nfa, and locate epsilon transitions
    auto transition_iter = nfa.transitions.find(state);
    auto epsilon_associations = transition_iter->second.find('-');

    //another base case. if this state doesn't have epislon transitions, return
    if (epsilon_associations == end(transition_iter->second)) return;

    //there are epsilon transitions to certain states. add them to the state list,
    //and epsilon check them
    for(auto i : epsilon_associations->second) {
        states.insert(i);
        epsilon_check(i, states, nfa, 0);
    }
}

//a function for printing a dfa to a file
void DFA::print_to_file(string file_name) const {
    ofstream outfile;
    outfile.open (file_name + ".dfa");

    //list of states
    outfile << string_states() << endl;
    //list of symbols
    outfile << string_alphabet() << endl;
    //start states
    outfile << string_start_states() << endl;
    //valid accept states
    outfile << string_accept_states() << endl;
    //transition function
    for (auto transition_str : string_transitions_vec()) {
        outfile << transition_str << " " << endl;
    }

    outfile.close();
}

//helper function to stringify all states
string DFA::string_states() const {
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

    return state_list;
}

//helper function to stringify alphabet
inline string DFA::string_alphabet() const {
    string alphabet_string = "";
    for (auto symbol : alphabet) {
        alphabet_string += symbol;
        alphabet_string += '\t';
    } 

    return alphabet_string;
}

//helper function to stringify start states
inline string DFA::string_start_states() const {
    string start_state_string = "{";
    for (auto states : start_state) {
        start_state_string += '0' + states;
        start_state_string += ',';
    }

    start_state_string[start_state_string.size() - 1] = '}';
    return start_state_string;
}

//helper function to stringify accept states
inline string DFA::string_accept_states() const {
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

    return accept_state_list;
}

//helper function to create a vector of strings representing transitions
vector<string> DFA::string_transitions_vec() const {
    vector<string> str_trans;
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
            str_trans.push_back(final_rep);
        }
    }

    return str_trans;
}

//main ------------------------------------------------------
int main (int argc, char** argv) {

    //look for a file from which to create nfa
    if (argc != 2) {
        cerr << "requires file name!" << endl;
        exit(EXIT_FAILURE);
    }

    NFA my_NFA = NFA(argv[1]); //create nfa from file
    DFA my_DFA = DFA(my_NFA); //create dfa from nfa

    //there wasn't a specification for naming the file the dfa prints to,
    //so using the name converted dfa. 
    my_DFA.print_to_file("converted_dfa"); //create a file

    return 0;
}

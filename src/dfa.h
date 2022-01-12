
class DFA {
    typedef unordered_set<int> dfa_state;

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

    public:
        DFA(const NFA &nfa);
};

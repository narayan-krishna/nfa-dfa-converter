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

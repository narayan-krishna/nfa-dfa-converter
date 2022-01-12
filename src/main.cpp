#include <iostream>
#include "nfa.h"
#include "dfa.h"

using namespace std;

int main (int argc, char **argv) {

    if (argc != 2) {
        cerr << "requires file name!" << endl;
        exit(EXIT_FAILURE);
    }

    NFA my_NFA = NFA(argv[1]); //create a dfa from a file
    DFA my_DFA = DFA(my_NFA); //construct a dfa using the nfa

    return 0;
}

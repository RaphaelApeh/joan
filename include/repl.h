#ifndef JOAN_REPL_H
#define JOAN_REPL_H

typedef enum {
    C_HELP,
    C_VERSION,
    C_REPL,
    C_RUN, // execute program
    C_UNKOWN,
    C_ERROR,
} CommandType;

struct Command {
    char* filename, error_msg; // Can be NULL
    CommandType type;
};

struct Command parse_args(char** args, int argc);
#endif
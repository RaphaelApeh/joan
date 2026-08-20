#ifndef JOAN_REPL_H
#define JOAN_REPL_H

typedef enum {
    C_ERROR = 0,
    C_HELP,
    C_VERSION,
    C_REPL,
    C_RUN,
    C_ITERATIVE,
    C_SCMD,
    C_TOKEN,
    C_UNKOWN,
} CommandType;

struct Command {
    char* filename, *error_msg;
    char* cmd_string;
    bool debug;
    CommandType type;
};

struct Command parse_args(char** args, int argc);
#endif
#include <string.h>
#include "Joan.h"
#include "repl.h"


typedef enum {
    INCOMPLETE, COMPLETE, OK // normal statement
} REPL_Type;


#define MAX_REPL_BUFF 1 << 10
static char buffer[MAX_REPL_BUFF] = {0};
static int buffer_count = 0;

struct Command parse_args(char** args, int argc)
{
    struct Command c = {0};
    if (argc == 0) // No Arguments
    {
        c.type = C_REPL;
        return c;
    }
    char* arg = args[0];
    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
    {
        c.type = C_HELP;
    } else if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0)
        c.type = C_VERSION;
    else {
        c.type = C_RUN;
    }
    return c;
}

static int incomplete_stmt(char* str, int len) 
{
    return str[len - 2] == '{' || str[len - 2] == '(';
}

static int complete_stmt(char* str, int len)
{
    return str[len - 2] == '}' || str[len - 2] == ')';
}

static REPL_Type parse_buffer(char* str)
{
    int len = strlen(str);
    REPL_Type type;
    if (incomplete_stmt(str, len))
        type = INCOMPLETE;
    else if (complete_stmt(str, len))
        type = COMPLETE;
    else
        type = OK;
    return type;
}

JN_API void Jn_repl(void)
{
    // TODO does not support nested statement.
    char line[256];
    int is_stmt = 0;
    fprintf(stderr, 
    "Joan  " JOAN_VERSION "\n"
    "!exit to exit for repl.\n"
    "!help for help info.\n"
    );
    Jn_program_init();
    for (;;)
    {
        fprintf(stderr, buffer_count == 0 ? ">>> " : "... ");
        if (!fgets(line, 256, stdin))
            break;
        strcat(buffer, line);
        buffer_count += strlen(line);
        if (strncmp(line, "!exit", 5) == 0) break;
        REPL_Type type =  parse_buffer(buffer);
        switch (type)
        {
            case INCOMPLETE:
                is_stmt = 1;
                break;
            case OK:
                if (is_stmt) break;
            case COMPLETE:
                Jn_exec_program(buffer);
                buffer[0] = 0;
                buffer_count = 0;
                is_stmt = 0;
                break;
        }
    }
    Jn_program_close();
}
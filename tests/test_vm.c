#include <Joan.h>

int main(void)
{
    Jn_State state = {0};
    Jn_program_init(&state, NULL, 0);
    Jn_pushinteger(&state, 1);
    Jn_program_close(&state);
    return 0;
}
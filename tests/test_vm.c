#include <Joan.h>

int main(void)
{
    J_State state = {0};
    Jn_program_init(&state);
    Jn_pushinteger(&state, 1);
    Jn_program_close(&state);
    return 0;
}
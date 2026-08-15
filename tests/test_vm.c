#include <Joan.h>

int main(void)
{
    Jn_State state = {0};
    Jn_program_init(&state, NULL, 0);
    Jn_pushinteger(&state, 4);
    Jn_pushinteger(&state, 3);
    Jn_setinst(&state, 1);
    Jn_setinst(&state, 65); // required to exit the vm. TODO
    int ret = Jn_exec(&state);
    JnObject* value = Jn_pop(&state);
    if (NULL != value)
        jn_obj_print(value); // 7
    putchar('\n');
    Jn_program_close(&state);
    return ret;
}
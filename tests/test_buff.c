#include <Joan.h>

int main(void)
{
    Jn_Buffer buff;
    Jn_buff_init(&buff);
    Jn_buff_add_char(&buff, 'H');
    Jn_buff_add_string(&buff, "ello World");
    Jn_buff_add_char(&buff, '\n');
    printf("buff = %.*s", (int)buff.len, buff.data);
    Jn_buff_clear(&buff);
    return 0;
}
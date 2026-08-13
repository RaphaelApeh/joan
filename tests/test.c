/*

MIT License

Copyright (c) 2026 Raphael Apeh

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


//  Simple C API Example

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <Joan.h>

const char* source = "printf(\"Hello from C !%s\", TEST)";

int main(void)
{
    J_State state = {0};
    Jn_program_init(&state);
    Jn_register(&state, "TEST", "TEST IF IT WORKS.", JN_RETURN_STRING(&state, "IT WORK's."));
    int exit_code = Jn_exec_program(&state, NULL, source);
    printf("Exit code = %d\n", exit_code);
    Jn_program_close(&state);
    return 0;
}
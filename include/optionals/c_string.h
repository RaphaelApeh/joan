/*

optionals/c_string.h Raphael Apeh

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
#ifndef C_STRING_H
#define C_STRING_H

#ifdef __cplusplus
extern "C" {
#endif
bool strends(const char* str, const char* suf);
bool strstarts(const char* str, const char* pre);
char* strrpl(const char* str, const char* old, const char* new);
char** strsplt(const char* str, char c, int* size);
char* strstrp(const char* str);
char* str_esc(const char* str);
int strpart(const char* str, char delim, char** left, char** right);
size_t strlen_utf8(const char* str);

// TODO
bool strstrcmp(char** src, char* src2);

#ifdef __cplusplus
}
#endif
#endif
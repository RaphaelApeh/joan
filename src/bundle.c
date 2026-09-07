/*

src/bundle.c Raphael Apeh

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

/*
 Unity build
 https://en.wikipedia.org/wiki/Unity_build

 Example:
 cc -O2 bundle.c main.c -o joan
*/

#include "arena.c"
#include "lexer.c"
#include "parse.c"
#include "token.h"
#include "ast.c"
#include "core.c"
#include "std.c"
#include "semantic.c"
#include "visits.c"
#include "helper.c"
#include "hashmap.c"
#include "gc.c"
#include "eval.c"
#include "object.c"
#include "repl.c"
#include "env.c"
#include "emit.c"
#include "cstring.c"
#include "checker.c"
#include "vm.c"

@echo off
:: TODO: add a target path dir. with opt debug
set opt = "" 

if "%1" == "debug" set opt = "-ggdb -DJOAN_DEBUG"
cd src

:: TODO: exclude main.c (might create more c files)
gcc %opt% -O2 -Wall -Wno-unused-function -Wno-unused-variable -I../include -c *.c

gcc %opt% -Wno-unused-function -Wno-unused-variable -shared -Wl,--output-def=libjoan.def -Wl,--out-implib=libjoan.a -Wl,--dll *.o -o libjoan.dll -static-libgcc -static

gcc -Wall  -Wno-unused-function -Wno-unused-variable -I../include -c main.c
gcc main.o -o joan.exe -static-libgcc -static -L%cd% -lm -ljoan

del *.o
cd ..
ar rcs template.a *.o

mkdir template
move /y src\joan.exe template
move /y src\libjoan* template


:: Clean-up
del src/joan.exe
del src/joan.a
del src/joan.dll


mkdir template\examples
mkdir template\extensions\vscode
copy extensions\joan\*.* template\extensions\vscode
copy include\Joan.h template
copy LICENSE template
copy README.md template
:: Examples
copy examples\condition.jt template\examples
:: TODO: More...
@echo off

cmake .. -G "MinGW Makefiles"
mingw32-make
:: back to base dir.
cd ..
pause
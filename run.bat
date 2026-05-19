@echo off

set FILE=%1
:: Change dir to build/
cd build
:: Execute Code
.\joan.exe "../examples/%FILE%"
cd ..
::NO need to pause program
::pause
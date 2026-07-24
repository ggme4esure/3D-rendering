@echo off
echo Compiling rendering.cpp...
g++ -O3 rendering.cpp -lgdi32 -luser32 -o rendering.exe

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful! Running rendering.exe...
    rendering.exe
) else (
    echo Compilation failed! See errors above.
    pause
)

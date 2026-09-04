@echo off
echo ==========================================
echo   Compiling rocket.cpp...
echo ==========================================
echo.

g++ -O3 rocket.cpp -lgdi32 -luser32 -o rocket.exe

if %ERRORLEVEL% EQU 0 (
    echo Compilation successful! Running rocket.exe...
    echo.
    start "" rocket.exe
) else (
    echo.
    echo Compilation failed! See errors above.
)

echo.
pause

@echo off
chcp 65001 > nul
cls

echo ========================================
echo   Compilando e executando 1.c
echo ========================================
echo.

gcc -O0 -o 1.exe 1.c -lpsapi

if %ERRORLEVEL% neq 0 (
    echo.
    echo Erro na compilacao!
    pause
    exit /b 1
)

echo.
echo Executando programa...
echo ========================================
echo.

1.exe

echo.
echo ========================================
pause

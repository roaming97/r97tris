@echo off
start /wait ./venv/Scripts/python.exe shader.py
if %errorlevel% == 2 (
    echo Error compiling shaders!
    exit
)
tcc ./src/main.c ./src/include/gl.c ./src/include/mt19937ar.c -Wall -o "tetris.exe" -lSDL2 -lbass -lSDL2main -Wl,-subsystem=windows
if %errorlevel% == 0 (
    .\tetris.exe
) else (
    echo Error compiling game!
    pause
)
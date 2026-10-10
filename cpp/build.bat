@echo off
REM Windows build (needs g++ from MinGW-w64 / MSYS2 on PATH)
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src\rules.cpp src\graph.cpp src\ml.cpp src\scorer.cpp src\generator.cpp src\io.cpp src\json_io.cpp src\sql_export.cpp src\config.cpp src\pipeline.cpp src\main.cpp -o detector.exe
if errorlevel 1 exit /b 1
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude src\rules.cpp src\graph.cpp src\ml.cpp src\scorer.cpp src\generator.cpp src\io.cpp src\json_io.cpp src\sql_export.cpp src\config.cpp src\pipeline.cpp tests\test_core.cpp -o test_core.exe
if errorlevel 1 exit /b 1
echo Build OK.  Run:  detector.exe   or   test_core.exe

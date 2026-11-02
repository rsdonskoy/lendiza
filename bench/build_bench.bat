@echo off
rem One-shot MSVC Release build of the self-contained bench.
rem   build_bench.bat <output.exe>
rem Mirrors the flags CMake applies to the release object library (/O2 /DNDEBUG).
setlocal
if "%~1"=="" (echo usage: build_bench.bat output.exe & exit /b 2)
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (echo vcvars64 not found & exit /b 1)
cd /d "%~dp0\.."
cl /nologo /std:c++20 /O2 /DNDEBUG /W3 /EHsc /I include /Fe:"%~1" bench\bench_decode.cpp src\lendiza.cpp
exit /b %errorlevel%

@echo off
rem Builds and runs the ASan truncation matrix with MSVC's AddressSanitizer.
rem Run from the tests/ directory:  tools\msvc_asan.cmd
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 >nul
if errorlevel 1 exit /b 2

cl /nologo /std:c++17 /fsanitize=address /EHsc /W4 /I..\include /I. asan_truncation.c ..\src\lendiza.cpp /Fe:asan_msvc.exe
if errorlevel 1 exit /b 2

set ASAN_OPTIONS=detect_leaks=0
asan_msvc.exe
exit /b %errorlevel%

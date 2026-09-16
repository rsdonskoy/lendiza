@echo off
rem check_kernel_compile.cmd - proves lendiza's core compiles for the Windows kernel
rem and that the resulting object pulls in no CRT runtime.
rem
rem   scripts\check_kernel_compile.cmd        (from the repository root)
rem
rem Requirements: Visual Studio 2022 + Windows Driver Kit (WDK).  Override the SDK
rem version with SET WDK_INC_VERSION=10.0.26100.0 if needed.

setlocal enabledelayedexpansion

set ROOT=%~dp0..
set VSROOT=C:\Program Files\Microsoft Visual Studio\2022
set VSDEV=%VSROOT%\Community\Common7\Tools\VsDevCmd.bat
if exist "%VSDEV%" goto have_vs
set VSDEV=%VSROOT%\Professional\Common7\Tools\VsDevCmd.bat
if exist "%VSDEV%" goto have_vs
set VSDEV=%VSROOT%\Enterprise\Common7\Tools\VsDevCmd.bat
if exist "%VSDEV%" goto have_vs
set VSDEV=%VSROOT%\BuildTools\Common7\Tools\VsDevCmd.bat
if exist "%VSDEV%" goto have_vs
echo [FAIL] VsDevCmd.bat not found - install Visual Studio 2022 with the C++ tools
exit /b 2

:have_vs

set WDK=C:\Program Files (x86)\Windows Kits\10
if exist "%WDK%\Include" goto have_wdk
echo [FAIL] WDK not found
exit /b 2

:have_wdk
if not "%WDK_INC_VERSION%"=="" goto have_ver
for /f "delims=" %%V in ('dir /b /ad /o:n "%WDK%\Include" ^| findstr /r "^[0-9]"') do set WDK_INC_VERSION=%%V
:have_ver
echo [info] using WDK include version %WDK_INC_VERSION%

call "%VSDEV%" -arch=x64 >nul
if errorlevel 1 exit /b 2

set KMINC=%WDK%\Include\%WDK_INC_VERSION%
set INC=/I"%ROOT%\include" /I"%ROOT%\kernel-examples\win" /I"%KMINC%\km\crt" /I"%KMINC%\km" /I"%KMINC%\shared"
set OUT=%TEMP%\lendiza_kernel_check
if not exist "%OUT%" mkdir "%OUT%"

echo.
echo === C++ core under /kernel (no CRT, no exceptions, no RTTI) ===
cl /nologo /kernel /std:c++17 /GR- /EHs-c- /W4 /Zc:preprocessor /D_AMD64_=1 /DWIN32=0x100 ^
   /D_WIN32_WINNT=0x0A00 /DNDEBUG %INC% /Fo"%OUT%\\" ^
   /c "%ROOT%\kernel-examples\win\lendiza_decode.cpp"
if errorlevel 1 (
    echo [FAIL] /kernel compile of the C++ shim failed
    exit /b 1
)

echo.
echo === C driver TU under /kernel ===
cl /nologo /kernel /W4 /D_AMD64_=1 /DWIN32=0x100 /D_WIN32_WINNT=0x0A00 /DNDEBUG %INC% ^
   /Fo"%OUT%\\" /c "%ROOT%\kernel-examples\win\lendizak.c"
if errorlevel 1 (
    echo [FAIL] /kernel compile of the C driver failed
    exit /b 1
)

echo.
echo === headered probe: does the library core alone compile in kernel mode? ===
cl /nologo /kernel /std:c++17 /GR- /EHs-c- /W4 /D_AMD64_=1 /DWIN32=0x100 /D_WIN32_WINNT=0x0A00 /DNDEBUG ^
   %INC% /Fo"%OUT%\\" /c "%ROOT%\tests\kernel_compile_probe.cpp"
if errorlevel 1 (
    echo [FAIL] kernel-mode compile of lendiza_core.hpp failed
    exit /b 1
)

echo.
echo === object must not reference the CRT ===
set BAD=0
for %%F in (lendiza_decode.obj lendizak.obj kernel_compile_probe.obj) do (
    dumpbin /symbols "%OUT%\%%F" > "%OUT%\%%F.syms"
    findstr /I /C:"__CxxFrameHandler" /C:"_purecall" /C:"__CxxThrowException" /C:"_initterm" ^
              /C:"__security_init_cookie" /C:"__std_terminate" /C:"??_R" "%OUT%\%%F.syms" >nul
    if not errorlevel 1 (
        echo [FAIL] %%F references CRT/RTTI/exception symbols:
        findstr /I /C:"__CxxFrameHandler" /C:"_purecall" /C:"__CxxThrowException" /C:"_initterm" ^
                 /C:"__security_init_cookie" /C:"__std_terminate" /C:"??_R" "%OUT%\%%F.syms"
        set BAD=1
    ) else (
        echo [ OK ] %%F clean
    )
)
if "%BAD%"=="1" exit /b 1

echo.
echo [PASS] lendiza core is kernel-compilable on Windows
exit /b 0

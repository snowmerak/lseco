@echo off
REM Lseco - Secure Memory Library
REM MSVC Build Script with Security Hardening

echo Building Lseco with MSVC...
echo.

REM Set compiler and flags
set CC=cl
set CFLAGS=/W4 /O2 /MD /GS /guard:cf /DLSECO_EXPORTS /DWIN32 /D_WINDOWS
set LDFLAGS=/DYNAMICBASE /NXCOMPAT /guard:cf

REM Source files
set SOURCES=secure_memory.c lseco_ffi.c
set LIB_NAME=lseco
set DLL_NAME=%LIB_NAME%.dll
set LIB_FILE=%LIB_NAME%.lib

echo.
echo Compiler Flags:
echo   /GS              - Stack Protector (Buffer Security Check)
echo   /guard:cf        - Control Flow Guard
echo   /DYNAMICBASE     - ASLR (Address Space Layout Randomization)
echo   /NXCOMPAT        - Data Execution Prevention (DEP)
echo.

REM Compile and link DLL
echo Building %DLL_NAME%...
%CC% %CFLAGS% /LD %SOURCES% /Fe%DLL_NAME% /link %LDFLAGS%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed!
    exit /b 1
)

echo.
echo Build successful!
echo   - %DLL_NAME%
echo   - %LIB_FILE%
echo.

REM Optional: Build test program
if exist test_lseco.c (
    echo Building test program...
    %CC% %CFLAGS% test_lseco.c %LIB_FILE% /Fetest_lseco.exe
    if %ERRORLEVEL% EQU 0 (
        echo Test program built: test_lseco.exe
        echo.
    )
)

echo Done!

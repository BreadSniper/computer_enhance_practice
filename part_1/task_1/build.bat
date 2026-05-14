@echo off

call "shell.bat"

echo ------------------------------------------------
echo Visual Studio environment is initialized from %B_VCVARSALL_PATH%
echo ------------------------------------------------

:: build
cl /std:c17 /Od /Zi main.c || exit /b 1

:: test
call :Test "listing_0037_single_register_mov" || exit /b 1
call :Test "listing_0038_many_register_mov" || exit /b 1

exit /b 0

:: tools

:Test
set "TEST_RESULT=0"

echo ------------------------------------------------
echo Running %~1 test

nasm.exe -f bin "%~1.asm" -o "%~1_original.bin" || goto TestFailed
main.exe "%~1_original.bin" "%~1_temp.asm" || goto TestFailed
nasm.exe -f bin "%~1_temp.asm" -o "%~1_rebuild.bin" || goto TestFailed
fc /b "%~1_original.bin" "%~1_rebuild.bin" >nul || goto TestFailed

echo Success
goto TestDone

:TestFailed
echo Failed
set "TEST_RESULT=1"

:TestDone
if exist "%~1_original.bin" del "%~1_original.bin"
if exist "%~1_rebuild.bin" del "%~1_rebuild.bin"
if exist "%~1_temp.asm" del "%~1_temp.asm"

echo ------------------------------------------------

exit /b %TEST_RESULT%
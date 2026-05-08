@echo off

call "shell.bat"

echo ------------------------------------------------
echo Visual Studio environment is initialized from %B_VCVARSALL_PATH%

echo ------------------------------------------------
echo Building signature...
echo ------------------------------------------------

cl /std:c17 /Od /Zi main.c
call :FailIfError 1

"../nasm/nasm.exe" listing_0037_single_register_mov.asm
"../nasm/nasm.exe" listing_0038_many_register_mov.asm

main.exe listing_0037_single_register_mov
main.exe listing_0038_many_register_mov



exit /b 0

:: Tools

:FailIfError
if errorlevel %1 ( exit 1 )
exit /b 0
@echo off

call "shell.bat"

pushd build
call devenv /Edit "%1" /DebugExe main.exe 
popd
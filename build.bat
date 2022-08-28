@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
mkdir build
pushd build
cl /Zi /std:c11 /Fe:ika.exe ..\src\*.c
popd
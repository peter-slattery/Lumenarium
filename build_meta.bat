@echo off

set ProjectDevFolder=%~dp0
set ProjectDevPath=%ProjectDevFolder:~0,-1%

pushd %ProjectDevPath%

IF NOT EXIST .\build\ mkdir .\build

C:\programs\ctime\ctime.exe -begin %ProjectDevPath%\build\win32_gs_meta_build_time.ctm

set CommonCompilerFlags=-nologo -DDEBUG=1 -DPLATFORM_WINDOWS -FC -WX -W4 -Z7 -Oi -GR- -EHsc -EHa- -MTd -fp:fast -fp:except-
set CommonCompilerFlags=-wd4127 -wd4702 -wd4101 -wd4505 -wd4100 -wd4189 -wd4244 -wd4201 -wd4996 -I%CommonLibs%  -O2 %CommonCompilerFlags%
set CommonLinkerFlags= -opt:ref 

pushd build

cl %CommonCompilerFlags% ..\meta\foldhaus_meta.cpp /link %CommonLinkerFlags%

C:\programs\ctime\ctime.exe -end %ProjectDevPath%\build\win32_gs_meta_build_time.ctm %LastError%
REM C:\programs\ctime\ctime.exe -stats %ProjectDevPath%\build\win32_gs_meta_build_time.ctm
popd
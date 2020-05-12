@echo off

SET MyPath=%~dp0
SET MyPath=%MyPath:~0,-1%
call %MyPath%\_prebuild_win32.bat meta debug msvc

set CommonCompilerFlags=-nologo -DDEBUG=1 -DPLATFORM_WINDOWS -FC -WX -W4 -Z7 -Oi -GR- -EHsc -EHa- -MTd -fp:fast -fp:except-
set CommonCompilerFlags=-wd4127 -wd4702 -wd4101 -wd4505 -wd4100 -wd4189 -wd4244 -wd4201 -wd4996 -I%CommonLibs%  -O2 %CommonCompilerFlags%
set CommonLinkerFlags= -opt:ref

pushd %BuildPath%

cl %CommonCompilerFlags% %SourceCodePath%\foldhaus_meta.cpp /link %CommonLinkerFlags%

popd

call %MyPath%\_postbuild_win32.bat
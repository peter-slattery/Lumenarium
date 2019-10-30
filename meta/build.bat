@echo off

set ProjectDevRoot=C:\projects
set ProjectName=foldhaus
set ProjectDevPath=%ProjectDevRoot%\%ProjectName%

pushd %ProjectDevPath%

IF NOT EXIST .\build\ mkdir .\build

C:\programs\ctime\ctime.exe -begin %ProjectDevPath%\build\win32_gs_meta_build_time.ctm

set CommonCompilerFlags=-nologo -DDEBUG=1 -DPLATFORM_WINDOWS -FC -WX -W4 -Z7 -Oi -GR- -EHsc -EHa- -MTd -fp:fast -fp:except-
set CommonCompilerFlags=-wd4127 -wd4702 -wd4101 -wd4505 -wd4100 -wd4189 -wd4244 -wd4201 -wd4996 -I%CommonLibs%  -O2 %CommonCompilerFlags%
set CommonLinkerFlags= -opt:ref 

pushd build

del *.pdb > NUL 2> NUL

REM cl %CommonCompilerFlags% ..\meta\main_meta.cpp /link %CommonLinkerFlags% user32.lib winmm.lib gdi32.lib -incremental:no
cl %CommonCompilerFlags% ..\meta\foldhaus_meta.cpp /link %CommonLinkerFlags%

C:\programs\ctime\ctime.exe -end C:\projects\foldhaus_meta\build\win32_gs_meta_build_time.ctm %LastError%
C:\programs\ctime\ctime.exe -stats C:\projects\foldhaus\build\win32_gs_meta_build_time.ctm
popd
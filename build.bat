@echo off

set ProjectDevRoot=C:\projects
set ProjectName=foldhaus
set ProjectDevPath=%ProjectDevRoot%\%ProjectName%

pushd %ProjectDevPath%

IF NOT EXIST .\build\ mkdir .\build

C:\programs\ctime\ctime.exe -begin %ProjectDevPath%\build\win32_foldhaus_build_time.ctm

set CommonCompilerFlags=-nologo -DDEBUG=1 -DPLATFORM_WINDOWS -FC -WX -W4 -Z7 -Oi -GR- -EHsc -EHa- -MTd -fp:fast -fp:except- -IC:\programs-dev\gs_libs\src
set CommonCompilerFlags=-wd4127 -wd4702 -wd4101 -wd4505 -wd4100 -wd4189 -wd4244 -wd4201 -wd4996 -I%CommonLibs%  -O2 %CommonCompilerFlags%
set CommonLinkerFlags= -opt:ref 

pushd build

del *.pdb > NUL 2> NUL

REM Run the Preprocessor
pushd ..\src\
..\build\foldhaus_meta.exe C:\projects\foldhaus\src\foldhaus_app.cpp
popd

REM cl %CommonCompilerFlags% F:\src\foldhaus_util_radialumia_file_converter.cpp /link %CommonLinkerFlags%
REM foldhaus_util_radialumia_file_converter.exe

echo WAITING FOR PDB TO WRITE > lock.tmp
cl %CommonCompilerFlags% ..\src\foldhaus_app.cpp /Fefoldhaus.dll /LD /link %CommonLinkerFlags% /EXPORT:InitializeApplication /EXPORT:UpdateAndRender /EXPORT:CleanupApplication /EXPORT:ReloadStaticData
set LastError=%ERRORLEVEL%
del lock.tmp

cl %CommonCompilerFlags% ..\src\win32_foldhaus.cpp /link %CommonLinkerFlags% user32.lib winmm.lib gdi32.lib  opengl32.lib dsound.lib Ws2_32.lib Comdlg32.lib -incremental:no

C:\programs\ctime\ctime.exe -end C:\projects\foldhaus\build\win32_foldhaus_build_time.ctm %LastError%
REM C:\programs\ctime\ctime.exe -stats C:\projects\foldhaus\build\win32_foldhaus_build_time.ctm
popd
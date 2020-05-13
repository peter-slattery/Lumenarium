@echo off

SET MyPath=%~dp0
SET MyPath=%MyPath:~0,-1%
call %MyPath%\_prebuild_win32.bat app debug msvc

set CommonCompilerFlags=-nologo -DDEBUG=1 -DPLATFORM_WINDOWS -FC -WX -W4 -Z7 -Oi -GR- -EHsc -EHa- -MTd -fp:fast -fp:except-
set CommonCompilerFlags=-wd4127 -wd4702 -wd4101 -wd4505 -wd4100 -wd4189 -wd4244 -wd4201 -wd4996 -O2 %CommonCompilerFlags%
set CommonLinkerFlags= -opt:ref -incremental:no

set DLLExports=/EXPORT:InitializeApplication /EXPORT:UpdateAndRender /EXPORT:CleanupApplication /EXPORT:ReloadStaticData

pushd %BuildPath%

del *.pdb > NUL 2> NUL

REM Run the Preprocessor
%MetaProgramPath%\foldhaus_meta.exe %SourceCodePath%\foldhaus_app.cpp

echo WAITING FOR PDB TO WRITE > lock.tmp

cl %CommonCompilerFlags% %SourceCodePath%\foldhaus_app.cpp /Fefoldhaus.dll /LD /link %CommonLinkerFlags% %DLLExports%
set LastError=%ERRORLEVEL%

del lock.tmp

cl %CommonCompilerFlags% %SourceCodePath%\win32_foldhaus.cpp /link %CommonLinkerFlags% user32.lib winmm.lib gdi32.lib  opengl32.lib dsound.lib Ws2_32.lib Comdlg32.lib

popd

call %MyPath%\_postbuild_win32.bat
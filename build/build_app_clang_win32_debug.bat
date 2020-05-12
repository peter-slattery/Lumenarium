@echo off

set ProjectDevFolder=%~dp0
set ProjectDevPath=%ProjectDevFolder:~0,-1%

pushd %ProjectDevPath%

IF NOT EXIST .\build_clang\ mkdir .\build_clang

C:\programs\ctime\ctime.exe -begin %ProjectDevPath%\build\win32_foldhaus_clang_build_time.ctm

set CommonCompilerFlags=-std=c++11 -Wno-writable-strings -Wno-unused-value -Wno-varargs -Wno-switch -Wno-microsoft-enum-forward-reference -DDEBUG=1

pushd .\build_clang\

REM Run the Preprocessor
foldhaus_meta.exe ..\src\foldhaus_app.cpp

echo WAITING FOR PDB TO WRITE > lock.tmp

clang %CommonCompilerFlags% ..\src\foldhaus_app.cpp -shared

set LastError=%ERRORLEVEL%

del lock.tmp

clang %CommonCompilerFlags% ..\src\win32_foldhaus.cpp -o win32_foldhaus.exe user32.lib winmm.lib gdi32.lib opengl32.lib dsound.lib Ws2_32.lib Comdlg32.lib

C:\programs\ctime\ctime.exe -end %ProjectDevPath%\build\win32_foldhaus_clang_build_time.ctm %LastError%
REM C:\programs\ctime\ctime.exe -stats %ProjectDevPath%\build\win32_foldhaus_clang_build_time.ctm
popd


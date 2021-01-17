@echo off

SET MyPath=%~dp0
SET MyPath=%MyPath:~0,-1%
call %MyPath%\_prebuild_win32.bat app debug clang

set CommonCompilerFlags=-std=c++11 -Wno-writable-strings -Wno-unused-value -Wno-varargs -Wno-switch -Wno-microsoft-enum-forward-reference -DDEBUG=1

pushd %BuildPath%

del *.pdb > NUL 2> NUL

echo WAITING FOR PDB TO WRITE > lock.tmp

clang++ %CommonCompilerFlags% %SourceCodePath%\foldhaus_app.cpp -shared -o

set LastError=%ERRORLEVEL%

del lock.tmp

clang++ -c %CommonCompilerFlags% %SourceCodePath%\platform_win32\win32_foldhaus.cpp
link win32_foldhaus.o user32.lib winmm.lib gdi32.lib opengl32.lib dsound.lib Ws2_32.lib Comdlg32.lib

popd

call %MyPath%\_postbuild_win32.bat

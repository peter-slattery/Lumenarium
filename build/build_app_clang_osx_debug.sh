pushd build
clang++ ../src/app/platform_osx/gs_osx.mm \
-g \
-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include \
-Wno-c11-extensions -Wno-unused-variable -Wno-unused-function -Wno-switch \
-framework Cocoa -framework OpenGL \
-o osx_foldhaus.out 
popd
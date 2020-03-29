pushd build
clang++ ../src/gs_osx.mm \
-g \
-Wno-c11-extensions -Wno-unused-variable -Wno-unused-function \
-framework Cocoa -framework OpenGL \
-o osx_foldhaus.out 
popd
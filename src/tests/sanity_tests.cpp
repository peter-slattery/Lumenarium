//
// File: sanity_tests.cpp
// Author: Peter Slattery
// Creation Date: 2021-03-06
//
#ifndef SANITY_TESTS_CPP

#include <stdio.h>
#include "../gs_libs/gs_types.h"
#include "../gs_libs/gs_types.cpp"
#include "../gs_libs/gs_tests.h"

#include "../gs_libs/gs_path.h"

gs_memory_arena Scratch = {};
void* Alloc(u64 Size, u64* ResultSize) { *ResultSize = Size; return malloc(Size); }
void Free(void* Ptr, u64 Size) { return free(Ptr); }

bool PathTest (char* In, char* Out) {
    return StringsEqual(SanitizePath(ConstString(In), &Scratch), ConstString(Out));
}

int main (int ArgCount, char** Args)
{
    Scratch = CreateMemoryArena(CreateAllocator(Alloc, Free));
    
    Test("gs_path.h")
    {
        TestResult(PathTest(".", "."));
        TestResult(PathTest(".\\", ".\\"));
        TestResult(PathTest("./", ".\\"));
        TestResult(PathTest("./../", "..\\"));
        TestResult(PathTest("C:/users/pslattery\\test.foo", "C:\\users\\pslattery\\test.foo"));
        TestResult(PathTest("./test/../foo.bar", ".\\foo.bar"));
        TestResult(PathTest("C:\\hello\\world\\.\\test", "C:\\hello\\world\\test"));
    }
    
    return 0;
}


#define SANITY_TESTS_CPP
#endif // SANITY_TESTS_CPP
//
// File: sanity_tests.cpp
// Author: Peter Slattery
// Creation Date: 2021-03-06
//
#ifndef SANITY_TESTS_CPP

#include <windows.h>
#include <stdio.h>
#include "../gs_libs/gs_types.h"
#include "../gs_libs/gs_types.cpp"
#include "../gs_libs/gs_tests.h"

#include "../gs_libs/gs_path.h"
#include "../gs_libs/gs_csv.h"

#include "./memory_arena_tests.cpp"

gs_memory_arena Scratch = {};
void* Alloc(u64 Size, u64* ResultSize) { *ResultSize = Size; return malloc(Size); }
void Free(void* Ptr, u64 Size) { return free(Ptr); }

bool StringTest (gs_const_string StrA, gs_const_string StrB)
{
  return StringsEqual(StrA, StrB);
}
bool StringTest (gs_string StrA, gs_string StrB)
{
  return StringsEqual(StrA, StrB);
}

bool PathTest (char* In, char* Out) {
  return StringsEqual(SanitizePath(ConstString(In), &Scratch), ConstString(Out));
}

global char* SampleCSV = R"FOO(Flower	Primary Hue (0-365)	Secondary Hue  (0-365)	Tertiary Hue  (0-365)	Homonyms
Flower A	55	32	128	foo, bar, blah, baz, whatever
Flower B	123	344	32	foo, bar, blah, baz, whatever
Flower C	55	32	128	foo, bar, blah, baz, whatever)FOO";

int main (int ArgCount, char** Args)
{
  Scratch = CreateMemoryArena(CreateAllocator(Alloc, Free), "Scratch");
  
  Test("gs_string")
  {
    gs_string TestString = PushStringF(&Scratch, 256, "Hello there, Sailor!");
    
    NullTerminate(&TestString);
    TestResult(IsNullTerminated(TestString));
    
    TestResult(StringTest(GetStringPrefix(TestString.ConstString, 5), ConstString("Hello")));
    TestResult(StringTest(GetStringPostfix(TestString.ConstString, 5), ConstString("ilor!")));
    TestResult(StringTest(GetStringAfter(TestString.ConstString, 13), ConstString("Sailor!")));
    TestResult(StringTest(GetStringBefore(TestString.ConstString, 5), ConstString("Hello")));
    TestResult(StringTest(Substring(TestString.ConstString, 5, 11), ConstString(" there")));
    
    TestResult(FindFirst(TestString, 5, 'l') == 16);
    TestResult(FindFirst(TestString, 0, 'k') == -1);
    TestResult(FindLast(TestString, 10, 'l') == 3);
    TestResult(FindLast(TestString, 'k') == -1);
    
    TestResult(FindFirstFromSet(TestString.ConstString, "re") == 1);
    TestResult(FindFirstFromSet(TestString.ConstString, "er") == 1);
    TestResult(FindFirstFromSet(TestString.ConstString, "bk") == -1);
    TestResult(FindFirstFromSet(TestString.ConstString, "ek") == 1);
    
    TestResult(FindLastFromSet(TestString.ConstString, "re") == 18);
    TestResult(FindLastFromSet(TestString.ConstString, "er") == 18);
    TestResult(FindLastFromSet(TestString.ConstString, "bk") == -1);
    TestResult(FindLastFromSet(TestString.ConstString, "rk") == 18);
    
    TestResult(StringContains(TestString.ConstString, ','));
    TestResult(!StringContains(TestString.ConstString, '@'));
    TestResult(StringsEqual(TestString, TestString));
    
    TestResult(StringEqualsCharArray(TestString, "Hello there, Sailor!"));
    TestResult(!StringEqualsCharArray(TestString, "Hello there, Sailor"));
    TestResult(!StringEqualsCharArray(TestString, "Foobar"));
    
    ReverseStringInPlace(&TestString);
    TestResult(StringTest(TestString, MakeString("!roliaS ,ereht olleH")));
    ReverseStringInPlace(&TestString);
    
    TestResult(ParseUInt(ConstString("532")) == 532);
    TestResult(ParseInt(ConstString("-1234567890")) == -1234567890);
    TestResult(ParseFloat(ConstString("-12345.6789")) == -12345.6789);
    TestResult(ParseFloat(ConstString("-1")) == -1);
    TestResult(ParseFloat(ConstString("-.035")) == -.035);
    
    TestString.Length = 0;
    U64ToASCII(&TestString, 53298, 10);
    TestResult(StringTest(TestString.ConstString, ConstString("53298")));
    
    TestString.Length = 0;
    R64ToASCII(&TestString, -145732.321, 2);
    TestResult(StringTest(TestString.ConstString, ConstString("-145732.32")));
  }
  
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
  
  Test("gs_csv.h")
  {
    gs_const_string TestCSV = ConstString(SampleCSV);
    gscsv_sheet Sheet = CSV_Parse(TestCSV, { '\t' }, &Scratch);
    
    gs_const_string Cell = CSVSheet_GetCell(Sheet, 0, 0);
    TestResult(StringsEqual(Cell, ConstString("Flower")));
    
    Cell = CSVSheet_GetCell(Sheet, 1, 1);
    TestResult(StringsEqual(Cell, ConstString("55")));
    
    Cell = CSVSheet_GetCell(Sheet, 4, 1);
    TestResult(StringsEqual(Cell, ConstString("foo, bar, blah, baz, whatever")));
    
    Cell = CSVSheet_GetCell(Sheet, 4, 3);
    TestResult(StringsEqual(Cell, ConstString("foo, bar, blah, baz, whatever")));
  }
  
  MemoryArenaTests();
  
  return 0;
}


#define SANITY_TESTS_CPP
#endif // SANITY_TESTS_CPP
//
// File: gs_tests.h
// Author: Peter Slattery
// Creation Date: 2021-03-06
//
#ifndef GS_TESTS_H

int CStringLen(char* Str)
{
    char* At = Str;
    while (*At != 0) { At++; }
    int Result = At - Str;
    return Result;
}

struct test_ctx
{
    int TestsCount;
    int TestsPassedCount;
};

static test_ctx TestCtx = {0};

static void
BeginTest(char* Name)
{
    int Length = CStringLen(Name);
    int Spaces = 25 - Length;
    if(Spaces < 0)
    {
        Spaces = 0;
    }
    printf("\"%s\" %.*s [", Name, Spaces, "------------------------------");
    TestCtx.TestsCount = 0;
    TestCtx.TestsPassedCount = 0;
}

static void
TestResult(bool Result)
{
    TestCtx.TestsCount += 1;
    if (Result) {
        TestCtx.TestsPassedCount += 1;
    }
    printf(Result ? "." : "X");
}

static void
EndTest(void)
{
    int Spaces = 10 - TestCtx.TestsCount;
    if(Spaces < 0) { Spaces = 0; }
    printf("]%.*s ", Spaces, "                                      ");
    printf("[%i/%i] %i passed, %i tests, ",
           TestCtx.TestsPassedCount, TestCtx.TestsCount,
           TestCtx.TestsPassedCount, TestCtx.TestsCount);
    if(TestCtx.TestsCount == TestCtx.TestsPassedCount)
    {
        printf("SUCCESS ( )");
    }
    else
    {
        printf("FAILED  (X)");
    }
    printf("\n");
}

#define Test(name) for(int _i_ = (BeginTest(name), 0); !_i_; _i_ += 1, EndTest())


#define GS_TESTS_H
#endif // GS_TESTS_H
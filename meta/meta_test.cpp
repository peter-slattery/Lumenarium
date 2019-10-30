typedef float r32;

struct test_struct
{
    float Float;
    int Int;
    char* CharPointer;
};

struct nested_struct
{
    float Hello;
    test_struct NestedValue;
    r32 TypedefedValue;
};

int main(char* Args, int ArgCount)
{
    return 0;
}


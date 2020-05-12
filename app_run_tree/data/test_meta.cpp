struct hello_struct_t
{
    const int x;
    float* y;
    char hello;
};

union Vector
{
    struct {
        int X;
        int Y;
        int Z;
    };
    int E[3];
};

struct test_def;

int main (int ArgCount, char* Args[])
{
    return 0;
}
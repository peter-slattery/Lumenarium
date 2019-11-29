struct animation_block
{
    r32 StartTime;
    r32 EndTime;
    animation_block* Next;
};

struct animation_layer
{
    animation_block* Blocks;
};
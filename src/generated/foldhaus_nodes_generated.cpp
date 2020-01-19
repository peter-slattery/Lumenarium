enum node_type
{
NodeType_Count,
};

node_specification NodeSpecifications[] = {
};
s32 NodeSpecificationsCount = 0;

internal void CallNodeProc(u32 SpecificationIndex, u8* Data, led* LEDs, s32 LEDsCount, r32 DeltaTime)
{
node_specification Spec = NodeSpecifications[SpecificationIndex];
switch (Spec.Type)
{
}
}

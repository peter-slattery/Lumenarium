node_struct_member MemberList_multiply_data[] = {
    { MemberType_r32, "A", (u64)&(((multiply_data*)0)->A), IsInputMember},
    { MemberType_r32, "B", (u64)&(((multiply_data*)0)->B), IsInputMember},
    { MemberType_r32, "Result", (u64)&(((multiply_data*)0)->Result), IsOutputMember},
};

enum node_type
{
    NodeType_MultiplyNodeProc,
};

node_specification NodeSpecifications[] = {
    { NodeType_MultiplyNodeProc, "Multiply", 8, MemberList_multiply_data, 3, 2, 1},
};
s32 NodeSpecificationsCount = 1;

internal void
CallNodeProc (interface_node* Node, u8* Data)
{
    switch (Node->Type)
    {
        case NodeType_MultiplyNodeProc:
        {
            MultiplyNodeProc((multiply_data*)Data);
        } break;
    }
}
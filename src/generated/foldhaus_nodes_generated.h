enum node_type
{
    NodeType_RevolvingDiscs,
    NodeType_SolidColorProc,
    NodeType_VerticalColorFadeProc,
    NodeType_Count,
};

static node_specification_ NodeSpecifications[] = {
{ NodeType_RevolvingDiscs, {"RevolvingDiscs", 14}, gsm_StructType_revolving_discs_data }, 
{ NodeType_SolidColorProc, {"SolidColorProc", 14}, gsm_StructType_solid_color_data }, 
{ NodeType_VerticalColorFadeProc, {"VerticalColorFadeProc", 21}, gsm_StructType_vertical_color_fade_data }, 
};

void CallNodeProc(node_type Type, u8* NodeData)
{
    switch(Type) { 
        case NodeType_RevolvingDiscs:
        {
            RevolvingDiscs((revolving_discs_data*)NodeData);
        } break;
        case NodeType_SolidColorProc:
        {
            SolidColorProc((solid_color_data*)NodeData);
        } break;
        case NodeType_VerticalColorFadeProc:
        {
            VerticalColorFadeProc((vertical_color_fade_data*)NodeData);
        } break;
    }
}


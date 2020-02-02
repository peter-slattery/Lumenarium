enum node_type
{
    NodeType_RevolvingDiscs,
    NodeType_VerticalColorFadeProc,
    NodeType_SolidColorProc,
    NodeType_Count,
};

void CallNodeProc(node_type Type, u8* NodeData)
{
    switch(Type) { 
        case NodeType_RevolvingDiscs:
        {
            RevolvingDiscs((revolving_discs_data*)NodeData);
        } break;
        case NodeType_VerticalColorFadeProc:
        {
            VerticalColorFadeProc((vertical_color_fade_data*)NodeData);
        } break;
        case NodeType_SolidColorProc:
        {
            SolidColorProc((solid_color_data*)NodeData);
        } break;
    }
}


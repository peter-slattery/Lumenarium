#define GENERATED_NODE_TYPES \
NodeType_MultiplyNodeProc, \
NodeType_AddNodeProc, \
NodeType_FloatValueProc, \
NodeType_SolidColorProc, \
NodeType_VerticalColorFadeProc

#define GENERATED_NODE_SPECS \
{ NodeType_MultiplyNodeProc, "MultiplyNodeProc", 16, MemberList_multiply_data, 12, 3, 2, 1, false}, \
{ NodeType_AddNodeProc, "AddNodeProc", 11, MemberList_add_data, 48, 3, 2, 1, false}, \
{ NodeType_FloatValueProc, "FloatValueProc", 14, MemberList_float_value_data, 8, 2, 1, 1, false}, \
{ NodeType_SolidColorProc, "SolidColorProc", 14, MemberList_solid_color_data, 16, 1, 1, 0, true}, \
{ NodeType_VerticalColorFadeProc, "VerticalColorFadeProc", 21, MemberList_vertical_color_fade_data, 24, 3, 3, 0, true}
#define GENERATED_NODE_SPECS_COUNT 5

#define EVALUATE_GENERATED_NODES \
case NodeType_MultiplyNodeProc: { MultiplyNodeProc((multiply_data*)NodeData); } break; \
case NodeType_AddNodeProc: { AddNodeProc((add_data*)NodeData); } break; \
case NodeType_FloatValueProc: { FloatValueProc((float_value_data*)NodeData); } break; \
case NodeType_SolidColorProc: { SolidColorProc((solid_color_data*)NodeData, LEDs, Colors, LEDCount); } break; \
case NodeType_VerticalColorFadeProc: { VerticalColorFadeProc((vertical_color_fade_data*)NodeData, LEDs, Colors, LEDCount); } break; \


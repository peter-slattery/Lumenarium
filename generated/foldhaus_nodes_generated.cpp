enum node_type
{
NodeType_OutputNode,
NodeType_MultiplyNodeProc,
NodeType_AddNodeProc,
NodeType_FloatValueProc,
NodeType_SolidColorProc,
NodeType_SinWaveProc,
NodeType_VectorProc,
NodeType_MultiplyPatterns,
NodeType_VerticalColorFadeProc,
NodeType_Count,
};

node_struct_member MemberList_multiply_data[] = {
{ MemberType_r32, "A", (u64)&((multiply_data*)0)->A, IsInputMember  },
{ MemberType_r32, "B", (u64)&((multiply_data*)0)->B, IsInputMember  },
{ MemberType_r32, "Result", (u64)&((multiply_data*)0)->Result,   IsOutputMember},
};

node_struct_member MemberList_add_data[] = {
{ MemberType_v4, "A", (u64)&((add_data*)0)->A, IsInputMember  },
{ MemberType_v4, "B", (u64)&((add_data*)0)->B, IsInputMember  },
{ MemberType_v4, "Result", (u64)&((add_data*)0)->Result,   IsOutputMember},
};

node_struct_member MemberList_float_value_data[] = {
{ MemberType_r32, "Value", (u64)&((float_value_data*)0)->Value, IsInputMember  },
{ MemberType_r32, "Result", (u64)&((float_value_data*)0)->Result,   IsOutputMember},
};

node_struct_member MemberList_solid_color_data[] = {
{ MemberType_v4, "Color", (u64)&((solid_color_data*)0)->Color, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "LEDs", (u64)&((solid_color_data*)0)->LEDs, IsInputMember | IsOutputMember},
};

node_struct_member MemberList_sin_wave_data[] = {
{ MemberType_r32, "Period", (u64)&((sin_wave_data*)0)->Period, IsInputMember  },
{ MemberType_r32, "Min", (u64)&((sin_wave_data*)0)->Min, IsInputMember  },
{ MemberType_r32, "Max", (u64)&((sin_wave_data*)0)->Max, IsInputMember  },
{ MemberType_r32, "Result", (u64)&((sin_wave_data*)0)->Result,   IsOutputMember},
};

node_struct_member MemberList_vector_data[] = {
{ MemberType_r32, "X", (u64)&((vector_data*)0)->X, IsInputMember  },
{ MemberType_r32, "Y", (u64)&((vector_data*)0)->Y, IsInputMember  },
{ MemberType_r32, "Z", (u64)&((vector_data*)0)->Z, IsInputMember  },
{ MemberType_r32, "W", (u64)&((vector_data*)0)->W, IsInputMember  },
{ MemberType_v4, "Result", (u64)&((vector_data*)0)->Result,   IsOutputMember},
};

node_struct_member MemberList_multiply_patterns_data[] = {
{ MemberType_NODE_COLOR_BUFFER, "ALEDs", (u64)&((multiply_patterns_data*)0)->ALEDs, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "BLEDs", (u64)&((multiply_patterns_data*)0)->BLEDs, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "ResultLEDs", (u64)&((multiply_patterns_data*)0)->ResultLEDs,   IsOutputMember},
};

node_struct_member MemberList_vertical_color_fade_data[] = {
{ MemberType_v4, "Color", (u64)&((vertical_color_fade_data*)0)->Color, IsInputMember  },
{ MemberType_r32, "Min", (u64)&((vertical_color_fade_data*)0)->Min, IsInputMember  },
{ MemberType_r32, "Max", (u64)&((vertical_color_fade_data*)0)->Max, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "ResultLEDs", (u64)&((vertical_color_fade_data*)0)->ResultLEDs,   IsOutputMember},
};

node_specification NodeSpecifications[] = {
{ NodeType_MultiplyNodeProc, "MultiplyNodeProc", 16, MemberList_multiply_data, 12, 3, false},
{ NodeType_AddNodeProc, "AddNodeProc", 11, MemberList_add_data, 48, 3, false},
{ NodeType_FloatValueProc, "FloatValueProc", 14, MemberList_float_value_data, 8, 2, false},
{ NodeType_SolidColorProc, "SolidColorProc", 14, MemberList_solid_color_data, 36, 2, false},
{ NodeType_SinWaveProc, "SinWaveProc", 11, MemberList_sin_wave_data, 16, 4, false},
{ NodeType_VectorProc, "VectorProc", 10, MemberList_vector_data, 32, 5, false},
{ NodeType_MultiplyPatterns, "MultiplyPatterns", 16, MemberList_multiply_patterns_data, 60, 3, false},
{ NodeType_VerticalColorFadeProc, "VerticalColorFadeProc", 21, MemberList_vertical_color_fade_data, 44, 4, false},
};
s32 NodeSpecificationsCount = 8;

internal void CallNodeProc(interface_node* Node, u8* Data, led* LEDs, s32 LEDsCount, r32 DeltaTime)
{
switch (Node->Type)
{
case NodeType_MultiplyNodeProc: { MultiplyNodeProc((multiply_data*)Data, DeltaTime); } break; 
case NodeType_AddNodeProc: { AddNodeProc((add_data*)Data, DeltaTime); } break; 
case NodeType_FloatValueProc: { FloatValueProc((float_value_data*)Data, DeltaTime); } break; 
case NodeType_SolidColorProc: { SolidColorProc((solid_color_data*)Data, DeltaTime); } break; 
case NodeType_SinWaveProc: { SinWaveProc((sin_wave_data*)Data, DeltaTime); } break; 
case NodeType_VectorProc: { VectorProc((vector_data*)Data, DeltaTime); } break; 
case NodeType_MultiplyPatterns: { MultiplyPatterns((multiply_patterns_data*)Data, DeltaTime); } break; 
case NodeType_VerticalColorFadeProc: { VerticalColorFadeProc((vertical_color_fade_data*)Data, DeltaTime); } break; 
}
}

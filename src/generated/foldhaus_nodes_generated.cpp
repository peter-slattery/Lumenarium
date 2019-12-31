enum node_type
{
NodeType_FloatValue,
NodeType_VectorValue,
NodeType_MultiplyNodeProc,
NodeType_AddNodeProc,
NodeType_SinWave,
NodeType_MultiplyPatterns,
NodeType_OutputNode,
NodeType_SolidColorProc,
NodeType_VerticalColorFadeProc,
NodeType_RevolvingDiscs,
NodeType_Count,
};

node_struct_member MemberList_float_value_data[] = {
{ MemberType_r32, "Value", (u64)&((float_value_data*)0)->Value, IsInputMember  },
{ MemberType_r32, "Result", (u64)&((float_value_data*)0)->Result,   IsOutputMember},
};

node_struct_member MemberList_vector_data[] = {
{ MemberType_r32, "X", (u64)&((vector_data*)0)->X, IsInputMember  },
{ MemberType_r32, "Y", (u64)&((vector_data*)0)->Y, IsInputMember  },
{ MemberType_r32, "Z", (u64)&((vector_data*)0)->Z, IsInputMember  },
{ MemberType_r32, "W", (u64)&((vector_data*)0)->W, IsInputMember  },
{ MemberType_v4, "Result", (u64)&((vector_data*)0)->Result,   IsOutputMember},
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

node_struct_member MemberList_sin_wave_data[] = {
{ MemberType_r32, "Period", (u64)&((sin_wave_data*)0)->Period, IsInputMember  },
{ MemberType_r32, "Min", (u64)&((sin_wave_data*)0)->Min, IsInputMember  },
{ MemberType_r32, "Max", (u64)&((sin_wave_data*)0)->Max, IsInputMember  },
{ MemberType_r32, "Result", (u64)&((sin_wave_data*)0)->Result,   IsOutputMember},
};

node_struct_member MemberList_multiply_patterns_data[] = {
{ MemberType_NODE_COLOR_BUFFER, "ALEDs", (u64)&((multiply_patterns_data*)0)->ALEDs, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "BLEDs", (u64)&((multiply_patterns_data*)0)->BLEDs, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "ResultLEDs", (u64)&((multiply_patterns_data*)0)->ResultLEDs,   IsOutputMember},
};

node_struct_member MemberList_output_node_data[] = {
{ MemberType_NODE_COLOR_BUFFER, "ResultLEDs", (u64)&((output_node_data*)0)->ResultLEDs, IsInputMember  },
};

node_struct_member MemberList_solid_color_data[] = {
{ MemberType_v4, "Color", (u64)&((solid_color_data*)0)->Color, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "ResultLEDs", (u64)&((solid_color_data*)0)->ResultLEDs,   IsOutputMember},
};

node_struct_member MemberList_vertical_color_fade_data[] = {
{ MemberType_v4, "Color", (u64)&((vertical_color_fade_data*)0)->Color, IsInputMember  },
{ MemberType_r32, "Min", (u64)&((vertical_color_fade_data*)0)->Min, IsInputMember  },
{ MemberType_r32, "Max", (u64)&((vertical_color_fade_data*)0)->Max, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "ResultLEDs", (u64)&((vertical_color_fade_data*)0)->ResultLEDs,   IsOutputMember},
};

node_struct_member MemberList_revolving_discs_data[] = {
{ MemberType_r32, "Rotation", (u64)&((revolving_discs_data*)0)->Rotation, IsInputMember  },
{ MemberType_r32, "ThetaZ", (u64)&((revolving_discs_data*)0)->ThetaZ, IsInputMember  },
{ MemberType_r32, "ThetaY", (u64)&((revolving_discs_data*)0)->ThetaY, IsInputMember  },
{ MemberType_r32, "DiscWidth", (u64)&((revolving_discs_data*)0)->DiscWidth, IsInputMember  },
{ MemberType_r32, "InnerRadius", (u64)&((revolving_discs_data*)0)->InnerRadius, IsInputMember  },
{ MemberType_r32, "OuterRadius", (u64)&((revolving_discs_data*)0)->OuterRadius, IsInputMember  },
{ MemberType_v4, "Color", (u64)&((revolving_discs_data*)0)->Color, IsInputMember  },
{ MemberType_NODE_COLOR_BUFFER, "ResultLEDs", (u64)&((revolving_discs_data*)0)->ResultLEDs,   IsOutputMember},
};

node_specification NodeSpecifications[] = {
{ NodeType_FloatValue, "FloatValue", 10, MemberList_float_value_data, 8, 2, false},
{ NodeType_VectorValue, "VectorValue", 11, MemberList_vector_data, 32, 5, false},
{ NodeType_MultiplyNodeProc, "MultiplyNodeProc", 16, MemberList_multiply_data, 12, 3, false},
{ NodeType_AddNodeProc, "AddNodeProc", 11, MemberList_add_data, 48, 3, false},
{ NodeType_SinWave, "SinWave", 7, MemberList_sin_wave_data, 20, 4, false},
{ NodeType_MultiplyPatterns, "MultiplyPatterns", 16, MemberList_multiply_patterns_data, 60, 3, false},
{ NodeType_OutputNode, "OutputNode", 10, MemberList_output_node_data, 20, 1, false},
{ NodeType_SolidColorProc, "SolidColorProc", 14, MemberList_solid_color_data, 36, 2, false},
{ NodeType_VerticalColorFadeProc, "VerticalColorFadeProc", 21, MemberList_vertical_color_fade_data, 44, 4, false},
{ NodeType_RevolvingDiscs, "RevolvingDiscs", 14, MemberList_revolving_discs_data, 60, 8, false},
};
s32 NodeSpecificationsCount = 10;

internal void CallNodeProc(u32 SpecificationIndex, u8* Data, led* LEDs, s32 LEDsCount, r32 DeltaTime)
{
node_specification Spec = NodeSpecifications[SpecificationIndex];
switch (Spec.Type)
{
case NodeType_FloatValue: { FloatValue((float_value_data*)Data, DeltaTime); } break; 
case NodeType_VectorValue: { VectorValue((vector_data*)Data, DeltaTime); } break; 
case NodeType_MultiplyNodeProc: { MultiplyNodeProc((multiply_data*)Data, DeltaTime); } break; 
case NodeType_AddNodeProc: { AddNodeProc((add_data*)Data, DeltaTime); } break; 
case NodeType_SinWave: { SinWave((sin_wave_data*)Data, DeltaTime); } break; 
case NodeType_MultiplyPatterns: { MultiplyPatterns((multiply_patterns_data*)Data, DeltaTime); } break; 
case NodeType_OutputNode: { OutputNode((output_node_data*)Data, DeltaTime); } break; 
case NodeType_SolidColorProc: { SolidColorProc((solid_color_data*)Data, DeltaTime); } break; 
case NodeType_VerticalColorFadeProc: { VerticalColorFadeProc((vertical_color_fade_data*)Data, DeltaTime); } break; 
case NodeType_RevolvingDiscs: { RevolvingDiscs((revolving_discs_data*)Data, DeltaTime); } break; 
}
}

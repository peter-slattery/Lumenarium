enum gsm_struct_type
{
    gsm_StructType_solid_color_data,
    gsm_StructType_v4,
    gsm_StructType_float,
    gsm_StructType_color_buffer,
    gsm_StructType_led,
    gsm_StructType_s32,
    gsm_StructType_pixel,
    gsm_StructType_u8,
    gsm_StructType_vertical_color_fade_data,
    gsm_StructType_r32,
    gsm_StructType_sin_wave_data,
    gsm_StructType_multiply_patterns_data,
    gsm_StructType_revolving_discs_data,
    gsm_StructType_Count,
};

static gsm_struct_member_type_info StructMembers_v4[] = {
{ "x", 1, (u64)&((v4*)0)->x },
{ "y", 1, (u64)&((v4*)0)->y },
{ "z", 1, (u64)&((v4*)0)->z },
{ "w", 1, (u64)&((v4*)0)->w },
{ "r", 1, (u64)&((v4*)0)->r },
{ "g", 1, (u64)&((v4*)0)->g },
{ "b", 1, (u64)&((v4*)0)->b },
{ "a", 1, (u64)&((v4*)0)->a },
{ "E", 1, (u64)&((v4*)0)->E },
};
static gsm_struct_member_type_info StructMembers_led[] = {
{ "Index", 5, (u64)&((led*)0)->Index },
{ "Position", 8, (u64)&((led*)0)->Position },
};
static gsm_struct_member_type_info StructMembers_pixel[] = {
{ "R", 1, (u64)&((pixel*)0)->R },
{ "G", 1, (u64)&((pixel*)0)->G },
{ "B", 1, (u64)&((pixel*)0)->B },
{ "Channels", 8, (u64)&((pixel*)0)->Channels },
};
static gsm_struct_member_type_info StructMembers_color_buffer[] = {
{ "LEDs", 4, (u64)&((color_buffer*)0)->LEDs },
{ "Colors", 6, (u64)&((color_buffer*)0)->Colors },
{ "LEDCount", 8, (u64)&((color_buffer*)0)->LEDCount },
};
static gsm_struct_member_type_info StructMembers_solid_color_data[] = {
{ "Color", 5, (u64)&((solid_color_data*)0)->Color },
{ "Result", 6, (u64)&((solid_color_data*)0)->Result },
};
static gsm_struct_member_type_info StructMembers_vertical_color_fade_data[] = {
{ "Color", 5, (u64)&((vertical_color_fade_data*)0)->Color },
{ "Min", 3, (u64)&((vertical_color_fade_data*)0)->Min },
{ "Max", 3, (u64)&((vertical_color_fade_data*)0)->Max },
{ "Result", 6, (u64)&((vertical_color_fade_data*)0)->Result },
};
static gsm_struct_member_type_info StructMembers_sin_wave_data[] = {
{ "Period", 6, (u64)&((sin_wave_data*)0)->Period },
{ "Min", 3, (u64)&((sin_wave_data*)0)->Min },
{ "Max", 3, (u64)&((sin_wave_data*)0)->Max },
{ "Result", 6, (u64)&((sin_wave_data*)0)->Result },
{ "Accumulator", 11, (u64)&((sin_wave_data*)0)->Accumulator },
};
static gsm_struct_member_type_info StructMembers_multiply_patterns_data[] = {
{ "A", 1, (u64)&((multiply_patterns_data*)0)->A },
{ "B", 1, (u64)&((multiply_patterns_data*)0)->B },
{ "Result", 6, (u64)&((multiply_patterns_data*)0)->Result },
};
static gsm_struct_member_type_info StructMembers_revolving_discs_data[] = {
{ "Rotation", 8, (u64)&((revolving_discs_data*)0)->Rotation },
{ "ThetaZ", 6, (u64)&((revolving_discs_data*)0)->ThetaZ },
{ "ThetaY", 6, (u64)&((revolving_discs_data*)0)->ThetaY },
{ "DiscWidth", 9, (u64)&((revolving_discs_data*)0)->DiscWidth },
{ "InnerRadius", 11, (u64)&((revolving_discs_data*)0)->InnerRadius },
{ "OuterRadius", 11, (u64)&((revolving_discs_data*)0)->OuterRadius },
{ "Color", 5, (u64)&((revolving_discs_data*)0)->Color },
{ "Result", 6, (u64)&((revolving_discs_data*)0)->Result },
};

static gsm_struct_type_info StructTypes[] = {
{ gsm_StructType_solid_color_data, "solid_color_data", 16, 36, 0, 0, StructMembers_solid_color_data, 2 },
{ gsm_StructType_v4, "v4", 2, 16, 0, 0, StructMembers_v4, 3 },
{ gsm_StructType_float, "float", 5, 4, 0, 0, 0, 0 },
{ gsm_StructType_color_buffer, "color_buffer", 12, 20, 0, 0, StructMembers_color_buffer, 3 },
{ gsm_StructType_led, "led", 3, 20, 0, 0, StructMembers_led, 2 },
{ gsm_StructType_s32, "s32", 3, 4, 0, 0, 0, 0 },
{ gsm_StructType_pixel, "pixel", 5, 3, 0, 0, StructMembers_pixel, 2 },
{ gsm_StructType_u8, "u8", 2, 1, 0, 0, 0, 0 },
{ gsm_StructType_vertical_color_fade_data, "vertical_color_fade_data", 24, 44, 0, 0, StructMembers_vertical_color_fade_data, 4 },
{ gsm_StructType_r32, "r32", 3, 4, 0, 0, 0, 0 },
{ gsm_StructType_sin_wave_data, "sin_wave_data", 13, 20, 0, 0, StructMembers_sin_wave_data, 5 },
{ gsm_StructType_multiply_patterns_data, "multiply_patterns_data", 22, 60, 0, 0, StructMembers_multiply_patterns_data, 3 },
{ gsm_StructType_revolving_discs_data, "revolving_discs_data", 20, 60, 0, 0, StructMembers_revolving_discs_data, 8 },
};
static gsm_u32 StructTypesCount = 13;

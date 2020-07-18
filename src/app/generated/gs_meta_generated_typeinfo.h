enum gsm_meta_tag_type
{
    MetaTag_panel_type_file_view,
    MetaTag_panel_type_node_graph,
    MetaTag_node_output,
    MetaTag_node_struct,
    MetaTag_panel_cleanup,
    MetaTag_node_input,
    MetaTag_panel_init,
    MetaTag_panel_type_animation_timeline,
    MetaTag_panel_commands,
    MetaTag_panel_type_sculpture_view,
    MetaTag_node_proc,
    MetaTag_panel_type_hierarchy,
    MetaTag_panel_type_profiler,
    MetaTag_panel_render,
    MetaTag_panel_type_dmx_view,
};
gsm_meta_tag MetaTaggs_strings[] = {
    { "panel_type_file_view", 20 },
    { "panel_type_node_graph", 21 },
    { "node_output", 11 },
    { "node_struct", 11 },
    { "panel_cleanup", 13 },
    { "node_input", 10 },
    { "panel_init", 10 },
    { "panel_type_animation_timeline", 29 },
    { "panel_commands", 14 },
    { "panel_type_sculpture_view", 25 },
    { "node_proc", 9 },
    { "panel_type_hierarchy", 20 },
    { "panel_type_profiler", 19 },
    { "panel_render", 12 },
    { "panel_type_dmx_view", 19 },
};
enum gsm_struct_type
{
    gsm_StructType_r32,
    gsm_StructType_solid_color_data,
    gsm_StructType_v4,
    gsm_StructType_float,
    gsm_StructType_color_buffer,
    gsm_StructType_pixel,
    gsm_StructType_u8,
    gsm_StructType_s32,
    gsm_StructType_revolving_discs_data,
    gsm_StructType_vertical_color_fade_data,
    gsm_StructType_Count,
};

static gsm_struct_member_type_info StructMembers_v4[] = {
    { "x", 1, (u64)&((v4*)0)->x, {}, 0},
    { "y", 1, (u64)&((v4*)0)->y, {}, 0},
    { "z", 1, (u64)&((v4*)0)->z, {}, 0},
    { "w", 1, (u64)&((v4*)0)->w, {}, 0},
    { "r", 1, (u64)&((v4*)0)->r, {}, 0},
    { "g", 1, (u64)&((v4*)0)->g, {}, 0},
    { "b", 1, (u64)&((v4*)0)->b, {}, 0},
    { "a", 1, (u64)&((v4*)0)->a, {}, 0},
    { "xy", 2, (u64)&((v4*)0)->xy, {}, 0},
    { "yz", 2, (u64)&((v4*)0)->yz, {}, 0},
    { "xyz", 3, (u64)&((v4*)0)->xyz, {}, 0},
    { "z", 1, (u64)&((v4*)0)->z, {}, 0},
    { "E", 1, (u64)&((v4*)0)->E, {}, 0},
};
static gsm_struct_member_type_info StructMembers_pixel[] = {
    { "R", 1, (u64)&((pixel*)0)->R, {}, 0},
    { "G", 1, (u64)&((pixel*)0)->G, {}, 0},
    { "B", 1, (u64)&((pixel*)0)->B, {}, 0},
    { "Channels", 8, (u64)&((pixel*)0)->Channels, {}, 0},
};
static gsm_struct_member_type_info StructMembers_color_buffer[] = {
    { "LedPositions", 12, (u64)&((color_buffer*)0)->LedPositions, {}, 0},
    { "Colors", 6, (u64)&((color_buffer*)0)->Colors, {}, 0},
    { "LEDCount", 8, (u64)&((color_buffer*)0)->LEDCount, {}, 0},
};
static gsm_struct_member_type_info StructMembers_solid_color_data[] = {
    { "Color", 5, (u64)&((solid_color_data*)0)->Color, {MetaTag_node_input, }, 1},
    { "Result", 6, (u64)&((solid_color_data*)0)->Result, {MetaTag_node_output, }, 1},
};
static gsm_struct_member_type_info StructMembers_revolving_discs_data[] = {
    { "Rotation", 8, (u64)&((revolving_discs_data*)0)->Rotation, {MetaTag_node_input, }, 1},
    { "ThetaZ", 6, (u64)&((revolving_discs_data*)0)->ThetaZ, {MetaTag_node_input, }, 1},
    { "ThetaY", 6, (u64)&((revolving_discs_data*)0)->ThetaY, {MetaTag_node_input, }, 1},
    { "DiscWidth", 9, (u64)&((revolving_discs_data*)0)->DiscWidth, {MetaTag_node_input, }, 1},
    { "InnerRadius", 11, (u64)&((revolving_discs_data*)0)->InnerRadius, {MetaTag_node_input, }, 1},
    { "OuterRadius", 11, (u64)&((revolving_discs_data*)0)->OuterRadius, {MetaTag_node_input, }, 1},
    { "Color", 5, (u64)&((revolving_discs_data*)0)->Color, {MetaTag_node_input, }, 1},
    { "Result", 6, (u64)&((revolving_discs_data*)0)->Result, {MetaTag_node_output, }, 1},
};
static gsm_struct_member_type_info StructMembers_vertical_color_fade_data[] = {
    { "Color", 5, (u64)&((vertical_color_fade_data*)0)->Color, {MetaTag_node_input, }, 1},
    { "Min", 3, (u64)&((vertical_color_fade_data*)0)->Min, {MetaTag_node_input, }, 1},
    { "Max", 3, (u64)&((vertical_color_fade_data*)0)->Max, {MetaTag_node_input, }, 1},
    { "Result", 6, (u64)&((vertical_color_fade_data*)0)->Result, {MetaTag_node_output, }, 1},
};

static gsm_struct_type_info StructTypes[] = {
    { gsm_StructType_r32, "r32", 3, 4, 0, 0, 0, 0 },
    { gsm_StructType_solid_color_data, "solid_color_data", 16, 32, 0, 0, StructMembers_solid_color_data, 2 },
    { gsm_StructType_v4, "v4", 2, 16, 0, 0, StructMembers_v4, 5 },
    { gsm_StructType_float, "float", 5, 4, 0, 0, 0, 0 },
    { gsm_StructType_color_buffer, "color_buffer", 12, 16, 0, 0, StructMembers_color_buffer, 3 },
    { gsm_StructType_pixel, "pixel", 5, 0, 0, 0, StructMembers_pixel, 2 },
    { gsm_StructType_u8, "u8", 2, 0, 0, 0, 0, 0 },
    { gsm_StructType_s32, "s32", 3, 0, 0, 0, 0, 0 },
    { gsm_StructType_revolving_discs_data, "revolving_discs_data", 20, 56, 0, 0, StructMembers_revolving_discs_data, 8 },
    { gsm_StructType_vertical_color_fade_data, "vertical_color_fade_data", 24, 40, 0, 0, StructMembers_vertical_color_fade_data, 4 },
};
static gsm_u32 StructTypesCount = 12;

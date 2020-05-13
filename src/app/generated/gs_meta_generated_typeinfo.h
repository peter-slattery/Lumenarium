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
gsm_meta_tag MetaTagStrings[] = {
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
    gsm_StructType_Count,
};


static gsm_struct_type_info StructTypes[] = {
};
static gsm_u32 StructTypesCount = 0;

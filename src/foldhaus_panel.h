
typedef struct panel panel;

#define PANEL_INIT_PROC(name) void name(panel* Panel)
typedef PANEL_INIT_PROC(panel_init_proc);

#define PANEL_CLEANUP_PROC(name) void name(panel* Panel)
typedef PANEL_CLEANUP_PROC(panel_cleanup_proc);

#define PANEL_RENDER_PROC(name) void name(panel Panel, v2 PanelMin, v2 PanelMax, render_command_buffer* RenderBuffer, app_state* State, context Context, mouse_state Mouse)
typedef PANEL_RENDER_PROC(panel_render_proc);

enum panel_split_direction
{
    PanelSplit_NoSplit,
PanelSplit_Horizontal,
    PanelSplit_Vertical,
    
    PanelSplit_Count,
};

typedef struct panel_entry panel_entry;

struct panel
{
panel_render_proc* Render;
panel_cleanup_proc* Cleanup;

    panel_split_direction SplitDirection;
    r32 SplitPercent;
    
    // TODO(Peter): This REALLY doesn't want to live here
    // Probably belongs in a more generalized PanelInterfaceState or something
    b32 PanelSelectionMenuOpen;
    
    union{
    panel_entry* Left;
        panel_entry* Top;
    };
    union{
        panel_entry* Right;
        panel_entry* Bottom;
        };
};

struct free_panel
{
    panel_entry* Next;
};

struct panel_entry
{
    panel Panel;
     free_panel Free;
};

#define PANELS_MAX 16
struct panel_layout
{
panel_entry Panels[PANELS_MAX];
    u32 PanelsUsed;
    
     panel_entry FreeList;
};

struct panel_definition
{
    char* PanelName;
    s32 PanelNameLength;
    panel_init_proc* Init;
    panel_cleanup_proc* Cleanup;
    panel_render_proc* Render;
};
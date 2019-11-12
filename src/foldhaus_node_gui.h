struct gui_node
{
    s32 Handle;
    node_type Type;
    v2 Min, Dim;
};

#define GUI_NODES_MAX 256
struct gui_node_list
{
    s32 NodesUsed;
    gui_node Nodes[GUI_NODES_MAX];
};

//
// File: foldhaus_node_gui.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_NODE_GUI_H

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


#define FOLDHAUS_NODE_GUI_H
#endif // FOLDHAUS_NODE_GUI_H
//
// File: foldhaus_node.h
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_NODE_H

typedef enum node_type node_type;

#define IsInputMember 1 << 0
#define IsOutputMember 1 << 1

#define DEFAULT_NODE_DIMENSION v2{125, 150}

struct color_buffer
{
    v4* LedPositions;
    pixel* Colors;
    s32 LEDCount;
};

// TODO(Peter): Use the Meta RTTI
// :UseMetaInfo
enum struct_member_type
{
    MemberType_Invalid,
    MemberType_s32,
    MemberType_r32,
    MemberType_v4,
    MemberType_NODE_COLOR_BUFFER,
    MemberTypeCount,
};

// TODO(Peter): Use the Meta RTTI
// :UseMetaInfo
struct node_struct_member
{
    struct_member_type Type;
    char* Name;
    u64 Offset;
    b32 IsInput;
};

// :UseMetaInfo
struct node_specification
{
    node_type Type;
    
    char* Name;
    s32 NameLength;
    
    node_struct_member* MemberList;
    u32 DataStructSize;
    u32 MemberListLength;
    
    b32 IsPattern;
};

struct node_specification_
{
    node_type Type;
    string Identifier;
    gsm_struct_type DataType;
};

struct pattern_node
{
    // TODO(Peter): Something to think about further down the line is the fact that
    // SpecificationIndex doesn't have to stay static throughout a single instance of
    // an application, let alone across separate runs. If you recompile (hot load or not)
    // with a new specification, the indecies all get thrown off. Should we hash the spec
    // names or something?
    
    // TODO(Peter): A more immediate thing to handle is that u32 should really be node_type
    u32 SpecificationIndex;
};

struct pattern_node_connection
{
    gs_list_handle UpstreamNodeHandle;
    gs_list_handle DownstreamNodeHandle;
    
    u32 UpstreamPortIndex;
    u32 DownstreamPortIndex;
};

struct pattern_node_workspace
{
    gs_list<pattern_node> Nodes;
    gs_bucket<pattern_node_connection> Connections;
    
    // This is storage for all the structures which follow.
    // It is cleared when new nodes are added so that the
    // acceleration structures can be recalculated
    memory_arena Storage;
    s32* SparseToSortedNodeMap;
    gs_list_handle* SortedNodeHandles;
};


///////////////////////////////////////////////
//   Pre Processor Macros
///////////////////////////////////////////////

#define NODE_STRUCT(data_name) \
struct data_name

#define NODE_PATTERN_STRUCT(data_name) \
struct data_name

#define NODE_PROC(proc_name, input_type) \
void proc_name(input_type* Data, r32 DeltaTime)

#define NODE_IN(type, name) type name
#define NODE_OUT(type, name) type name


///////////////////////////////////////////////
//   OUTPUT NODE
///////////////////////////////////////////////

struct output_node_data
{
    GSMetaTag(node_input);
    color_buffer Result;
};

void OutputNode(output_node_data* Data, r32 DeltaTime)
{
    
}

#define FOLDHAUS_NODE_H
#endif // FOLDHAUS_NODE_H
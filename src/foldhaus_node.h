typedef enum node_type node_type;

#define IsInputMember 1 << 0
#define IsOutputMember 1 << 1

#define DEFAULT_NODE_DIMENSION v2{125, 150}

#define NODE_COLOR_BUFFER \
led* LEDs; \
pixel* Colors; \
s32 LEDCount;

#define NAMED_NODE_COLOR_BUFFER(name) \
led* name##LEDs; \
pixel* name##Colors; \
s32 name##LEDCount;

#define NODE_COLOR_BUFFER_INOUT NODE_COLOR_BUFFER
#define NODE_COLOR_BUFFER_IN(name) NAMED_NODE_COLOR_BUFFER(name)
#define NODE_COLOR_BUFFER_OUT(name) NAMED_NODE_COLOR_BUFFER(name)

// TODO(Peter): Generate this
enum struct_member_type
{
    MemberType_Invalid,
    MemberType_s32,
    MemberType_r32,
    MemberType_v4,
    MemberType_NODE_COLOR_BUFFER,
    MemberTypeCount,
};

struct node_struct_member
{
    struct_member_type Type;
    char* Name;
    u64 Offset;
    b32 IsInput;
};

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

struct pattern_node
{
    // TODO(Peter): Something to think about further down the line is the fact that
    // SpecificationIndex doesn't have to stay static throughout a single instance of 
    // an application, let alone across separate runs. If you recompile (hot load or not)
    // with a new specification, the indecies all get thrown off. Should we hash the spec
    // names or something?
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

NODE_STRUCT(output_node_data)
{
    NODE_COLOR_BUFFER_IN(Result);
};

NODE_PROC(OutputNode, output_node_data)
{
    
}
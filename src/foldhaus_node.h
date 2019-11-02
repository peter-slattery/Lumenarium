typedef enum node_type node_type;

#define IsInputMember 1 << 0
#define IsOutputMember 1 << 1

#define DEFAULT_NODE_DIMENSION v2{125, 150}

#define NODE_COLOR_BUFFER \
led* LEDs; \
sacn_pixel* Colors; \
s32 LEDCount;

#define NAMED_NODE_COLOR_BUFFER(name) \
led* name##LEDs; \
sacn_pixel* name##Colors; \
s32 name##LEDCount;

#define NODE_COLOR_BUFFER_INOUT NODE_COLOR_BUFFER
#define NODE_COLOR_BUFFER_IN(name) NAMED_NODE_COLOR_BUFFER(name)
#define NODE_COLOR_BUFFER_OUT(name) NAMED_NODE_COLOR_BUFFER(name)

enum mouse_node_interaction
{
    NodeInteraction_None,
    NodeInteraction_MouseDragNode,
    NodeInteraction_MouseDragInput,
    NodeInteraction_KeyboardEnterPortValue,
    MouseNodeInteraction_Count,
};

enum node_port_direction
{
    NodePort_Input,
    NodePort_Output,
};

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

struct node_led_color_connection
{
    NODE_COLOR_BUFFER;
};

struct node_connection
{
    struct_member_type Type;
    // NOTE(Peter): Offset from the head of the node list that the connected node
    // is stored at. See GetNodeAtOffset for example of how this is used
    s32 UpstreamNodeOffset;
    s32 UpstreamNodePortIndex;
    s32 DownstreamNodeOffset;
    s32 DownstreamNodePortIndex;
    b32 DirectionMask;
    
    union
    {
        s32 S32Value;
        r32 R32Value;
        v4 V4Value;
        node_led_color_connection LEDsValue;
    };
};

// TODO(Peter): cant decide if this needs to be dynamic or just a really big number
// reevaluate once you have some examples
#define NODE_CONNECTIONS_MAX 8
struct interface_node
{
    string Name;
    
    v2 Min, Dim;
    
    s32 ConnectionsCount;
    node_connection* Connections;
    
    node_type Type;
    b32 UpdatedThisFrame;
    
    u8* PersistentData;
};

struct node_list
{
    u8* Memory;
    s32 Max;
    s32 Used;
    
    node_list* Next;
};

struct node_offset
{
    interface_node* Node;
    s32 Offset;
};

struct node_list_iterator
{
    node_list List;
    interface_node* At;
};

enum node_interaction_flag
{
    NodeInteraction_AllUpstream = 0x1,
    NodeInteraction_AllDownstream = 0x2,
};

struct node_interaction
{
    s32 NodeOffset;
    v2  MouseOffset;
    b32 Flags;
    
    // TODO(Peter): Inputs and outputs are all stored in the same array. Should this just be flags,
    // and we store the Port and Value?
    s32 InputPort;
    s32 InputValue;
    s32 OutputPort;
    s32 OutputValue;
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
    s32 DataStructSize;
    
    s32 MemberListLength;
    b32 IsPattern;
};

struct node_render_settings
{
    v4 PortColors[MemberTypeCount];
    bitmap_font* Font;
    b32 Display;
};

v4 DragButtonColors[] = {
    v4{.7f, .7f, .7f, 1},
    BlackV4,
    v4{.7f, .7f, .7f, 1},
};

#define NODE_HEADER_HEIGHT 20

#define NODE_PORT_X 20
#define NODE_PORT_Y 15
#define NODE_PORT_DIM v2{NODE_PORT_X, NODE_PORT_Y}
#define NODE_PORT_STEP NODE_PORT_Y + 20

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


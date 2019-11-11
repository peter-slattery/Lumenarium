inline s32
GetNodeMemorySize(s32 ConnectionsCount, s32 NameLength)
{
    s32 Result = sizeof(interface_node) + (sizeof(node_connection) * ConnectionsCount) + sizeof(NameLength);
    return Result;
}

inline s32
GetNodeMemorySize (interface_node Node)
{
    return GetNodeMemorySize(Node.ConnectionsCount, Node.Name.Length);
}

internal node_list_iterator
GetNodeListIterator(node_list List)
{
    node_list_iterator Result = {};
    Result.List = List;
    Result.CurrentBuffer = List.First;
    Result.At = (interface_node*)Result.CurrentBuffer->Memory;
    
    return Result;
}

internal b32
NodeIteratorIsValid(node_list_iterator Iter)
{
    b32 Result = (Iter.At != 0); 
    Result &= (((u8*)Iter.At - Iter.CurrentBuffer->Memory) < Iter.CurrentBuffer->Used);
    return Result;
}

internal void
Next (node_list_iterator* Iter)
{
    if (Iter->At->Handle == 0)
    {
        node_free_list_member* FreeNode = (node_free_list_member*)Iter->At;
        s32 SkipAmount = FreeNode->Size;
        Iter->At = (interface_node*)((u8*)Iter->At + SkipAmount);
    }
    else
    {
        s32 SkipAmount = GetNodeMemorySize(*Iter->At);
        if (((u8*)Iter->At - Iter->CurrentBuffer->Memory) + SkipAmount < Iter->CurrentBuffer->Used)
        {
            Iter->At = (interface_node*)((u8*)Iter->At + SkipAmount);
            if (Iter->At->Handle == 0) { Next(Iter); }
        }
        else if (Iter->CurrentBuffer->Next)
        {
            Iter->CurrentBuffer = Iter->CurrentBuffer->Next;
            Iter->At = (interface_node*)Iter->CurrentBuffer->Memory;
            if (Iter->At->Handle == 0) { Next(Iter); }
        }
        else
        {
            Iter->At = 0;
        }
    }
}

internal node_list_buffer*
AllocateNodeListBuffer (memory_arena* Storage, s32 Size)
{
    node_list_buffer* Result = PushStruct(Storage, node_list_buffer);;
    Result->Memory = PushSize(Storage, Size);
    Result->Max = Size;
    Result->Used = 0;
    Result->Next = 0;
    return Result;
}

internal node_list*
AllocateNodeList (memory_arena* Storage, s32 InitialSize)
{
    node_list* Result = PushStruct(Storage, node_list);
    Result->First = AllocateNodeListBuffer(Storage, InitialSize);
    Result->Head = Result->First;
    Result->TotalMax = InitialSize;
    Result->TotalUsed = 0;
    Result->HandleAccumulator = 0;
    return Result;
}

internal interface_node*
PushNodeOnList (node_list* List, s32 NameLength, s32 ConnectionsCount, v2 Min, v2 Dim, memory_arena* Storage)
{
    interface_node* Result = 0;
    
    s32 NodeMemorySize = GetNodeMemorySize(ConnectionsCount, NameLength);
    
    if (List->TotalUsed + NodeMemorySize >= List->TotalMax)
    {
        node_list_buffer* Buf = AllocateNodeListBuffer(Storage, List->Head->Max);
        List->Head->Next = Buf;
        List->Head = Buf;
        List->TotalMax += Buf->Max;
    }
    Assert(List->TotalUsed + NodeMemorySize <= List->TotalMax);
    
    Result = (interface_node*)(List->Head->Memory + List->Head->Used);
    Result->Handle = ++List->HandleAccumulator;
    Result->Name = MakeString((char*)(Result + 1), NameLength);
    
    Result->ConnectionsCount = ConnectionsCount;
    Result->Connections = (node_connection*)(Result->Name.Memory + NameLength);
    
    Result->Min = Min;
    Result->Dim = Dim;
    
    List->Head->Used += NodeMemorySize;
    List->TotalUsed += NodeMemorySize;
    
    return Result;
}

internal void
FreeNodeOnList (node_list* List, interface_node* Node)
{
    // TODO(Peter): 
}

internal void
InitializeNodeConnection (node_connection* Connection, struct_member_type Type, b32 DirectionMask)
{
    Connection->Type = Type;
    Connection->UpstreamNodeHandle = 0;
    Connection->UpstreamNodePortIndex = -1;
    Connection->DownstreamNodeHandle = 0;
    Connection->DownstreamNodePortIndex = -1;
    Connection->DirectionMask = DirectionMask;
    switch (Type)
    {
        case MemberType_s32:
        {
            Connection->S32Value = 0;
        }break;
        
        case MemberType_r32:
        {
            Connection->R32Value = 0;
        }break;
        
        case MemberType_v4:
        {
            Connection->V4Value = v4{0, 0, 0, 1};
        }break;
        
        case MemberType_NODE_COLOR_BUFFER:
        {
            Connection->LEDsValue = {};
        }break;
        
        InvalidDefaultCase;
    }
}

inline r32
CalculateNodeHeight (s32 Members)
{
    r32 Result = (NODE_PORT_STEP * Members) + NODE_HEADER_HEIGHT;
    return Result;
}

internal void
PushNodeOnListFromSpecification (node_list* List, node_specification Spec, v2 Min, memory_arena* Storage)
{
    // :NodesDontNeedToKnowTheirBounds
    r32 NodeHeight = CalculateNodeHeight (Spec.MemberListLength);
    interface_node* Node = PushNodeOnList(List, 
                                          Spec.NameLength, 
                                          Spec.MemberListLength,
                                          Min, 
                                          v2{150, NodeHeight}, 
                                          Storage);
    Node->Type = Spec.Type;
    
    CopyCharArrayToString(Spec.Name, Spec.NameLength, &Node->Name);
    
    node_struct_member* MemberList = Spec.MemberList;
    for (s32 MemberIdx = 0; MemberIdx < Spec.MemberListLength; MemberIdx++)
    {
        node_struct_member Member = MemberList[MemberIdx];
        InitializeNodeConnection(Node->Connections + MemberIdx, Member.Type, Member.IsInput);
    }
    
    Node->PersistentData = PushArray(Storage, u8, Spec.DataStructSize);
}

internal interface_node*
PushOutputNodeOnList (node_list* List, v2 Min, memory_arena* Storage)
{
    string OutputNodeName = MakeStringLiteral("Output");
    interface_node* Node = PushNodeOnList(List, 
                                          OutputNodeName.Length,
                                          1,
                                          Min,
                                          DEFAULT_NODE_DIMENSION,
                                          Storage);
    Node->Type = NodeType_OutputNode;
    CopyStringTo(OutputNodeName, &Node->Name);
    InitializeNodeConnection(Node->Connections, MemberType_NODE_COLOR_BUFFER, IsInputMember);
    
    return Node;
}

internal interface_node*
GetNodeWithHandle(node_list* List, s32 Handle)
{
    DEBUG_TRACK_FUNCTION;
    interface_node* Result = 0;
    
    node_list_iterator Iter = GetNodeListIterator(*List);
    while (NodeIteratorIsValid(Iter))
    {
        if(Iter.At->Handle == Handle)
        {
            Result = Iter.At;
            break;
        }
        Next(&Iter);
    }
    
    return Result;
}

internal rect
CalculateNodeBounds (interface_node* Node, node_render_settings Settings)
{
    rect Result = {};
    Result.Min = Node->Min;
    Result.Max = Node->Min + Node->Dim + v2{0, NODE_HEADER_HEIGHT};
    return Result;
}

internal rect
CalculateNodeInputPortBounds (interface_node* Node, s32 Index, node_render_settings RenderSettings)
{
    rect Result = {};
    
    Result.Min = v2{
        Node->Min.x,
        Node->Min.y + Node->Dim.y - ((NODE_PORT_STEP * (Index + 1)) + NODE_HEADER_HEIGHT)};
    Result.Max = Result.Min + NODE_PORT_DIM;
    
    return Result;
}

internal rect
CalculateNodeInputValueBounds (interface_node* Node, s32 Index, node_render_settings RenderSettings)
{
    rect Result = {};
    rect Port = CalculateNodeInputPortBounds(Node, Index, RenderSettings);
    Result.Min = v2{Port.Max.x, Port.Min.y};
    Result.Max = Result.Min + v2{NODE_PORT_DIM.x * 2, NODE_PORT_DIM.y};
    return Result;
}

internal rect
CalculateNodeOutputPortBounds (interface_node* Node, s32 Index, node_render_settings RenderSettings)
{
    rect Result = {};
    Result.Min = v2{
        Node->Min.x + Node->Dim.x - NODE_PORT_DIM.x,
        Node->Min.y + Node->Dim.y - ((NODE_PORT_STEP * (Index + 1)) + NODE_HEADER_HEIGHT)};
    Result.Max = Result.Min + NODE_PORT_DIM;
    return Result;
}

internal rect
CalculateNodeOutputValueBounds (interface_node* Node, s32 Index, node_render_settings RenderSettings)
{
    rect Result = {};
    rect Port = CalculateNodeOutputPortBounds(Node, Index, RenderSettings);
    Result.Min = v2{Port.Min.x - (NODE_PORT_DIM.x * 2), Port.Min.y};
    Result.Max = v2{Port.Min.x, Port.Max.y};
    return Result;
}

internal rect
GetBoundsOfPortConnectedToInput (interface_node* Node, s32 PortIndex, node_list* NodeList, node_render_settings RenderSettings)
{
    interface_node* ConnectedNode =
        GetNodeWithHandle(NodeList, Node->Connections[PortIndex].UpstreamNodeHandle);
    rect Result = CalculateNodeOutputPortBounds(ConnectedNode, Node->Connections[PortIndex].UpstreamNodePortIndex, RenderSettings);
    return Result;
}

internal rect
GetBoundsOfPortConnectedToOutput (interface_node* Node, s32 PortIndex, node_list* NodeList, node_render_settings RenderSettings)
{
    interface_node* ConnectedNode = GetNodeWithHandle(NodeList, Node->Connections[PortIndex].DownstreamNodeHandle);
    rect Result = CalculateNodeInputPortBounds(ConnectedNode, Node->Connections[PortIndex].DownstreamNodePortIndex, RenderSettings);
    return Result;
}

internal rect
CalculateNodeDragHandleBounds (rect NodeBounds, s32 Index, node_render_settings RenderSettings)
{
    rect Result {};
    v2 HorizontalOffset = v2{Width(NodeBounds) / 3, 0};
    Result.Min = v2{NodeBounds.Min.x, NodeBounds.Max.y - NODE_HEADER_HEIGHT} + (HorizontalOffset * Index);
    Result.Max = Result.Min + v2{HorizontalOffset.x, NODE_HEADER_HEIGHT};
    return Result;
}

internal node_interaction
NewEmptyNodeInteraction ()
{
    node_interaction Result = {};
    Result.NodeHandle = 0;
    Result.InputPort = -1;
    Result.InputValue = -1;
    Result.OutputPort = -1;
    Result.OutputValue = -1;
    return Result;
}

internal node_interaction
NewNodeInteraction (s32 NodeHandle, v2 MouseOffset)
{
    node_interaction Result = {};
    Result.NodeHandle = NodeHandle;
    Result.MouseOffset = MouseOffset;
    Result.InputPort = -1;
    Result.InputValue = -1;
    Result.OutputPort = -1;
    Result.OutputValue = -1;
    return Result;
}

internal b32
IsDraggingNode (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0) && 
                  (Interaction.OutputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodePort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) &&
                  (Interaction.InputPort >= 0 || Interaction.OutputPort >= 0) &&
                  (Interaction.InputValue < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) &&
                  (Interaction.InputPort < 0 && Interaction.OutputPort < 0) &&
                  (Interaction.InputValue >= 0 || Interaction.OutputValue >= 0));
    return Result;
}

internal b32
IsDraggingNodeInput (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.InputPort >= 0 || Interaction.InputValue >= 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeInputPort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.InputPort >= 0) && (Interaction.InputValue < 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeInputValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.InputPort < 0) &&  (Interaction.InputValue >= 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutput (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.OutputPort >= 0 || Interaction.OutputValue >= 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutputPort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.OutputPort >= 0) && (Interaction.OutputValue < 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutputValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeHandle > 0) && 
                  (Interaction.OutputPort < 0) &&  (Interaction.OutputValue >= 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
ConnectionIsConnected (node_connection Connection)
{
    b32 Result = (Connection.UpstreamNodeHandle > 0) || (Connection.DownstreamNodeHandle > 0);
    return Result;
}

internal b32
ConnectionIsConnected (interface_node* Node, s32 Index)
{
    b32 Result = ConnectionIsConnected(Node->Connections[Index]);
    return Result;
}

internal b32
ConnectionHasUpstreamConnection (node_connection Connection)
{
    b32 Result = (Connection.UpstreamNodeHandle > 0);
    return Result;
}

internal b32
ConnectionHasUpstreamConnection (interface_node* Node, s32 Index)
{
    b32 Result = ConnectionHasUpstreamConnection(Node->Connections[Index]);
    return Result;
}

internal b32
ConnectionHasDownstreamConnection (node_connection Connection)
{
    b32 Result = (Connection.DownstreamNodeHandle > 0);
    return Result;
}

internal b32
ConnectionHasDownstreamConnection (interface_node* Node, s32 Index)
{
    b32 Result = ConnectionHasDownstreamConnection(Node->Connections[Index]);
    return Result;
}

internal b32
ConnectionIsInput (node_connection Connection)
{
    b32 Result = (Connection.DirectionMask & IsInputMember) > 0;
    return Result;
}

internal b32
ConnectionIsInput (interface_node* Node, s32 ConnectionIdx)
{
    return ConnectionIsInput(Node->Connections[ConnectionIdx]);
}

internal b32
ConnectionIsOutput (node_connection Connection)
{
    b32 Result = (Connection.DirectionMask & IsOutputMember) > 0;
    return Result;
}

internal b32
ConnectionIsOutput (interface_node* Node, s32 ConnectionIdx)
{
    return ConnectionIsOutput(Node->Connections[ConnectionIdx]);
}

internal b32
CheckForRecursionWithHandle (node_list* NodeList, s32 LookForNodeHandle, interface_node* StartNode)
{
    DEBUG_TRACK_FUNCTION;
    b32 Result = false;
    
    for (s32 Connection = 0; Connection < StartNode->ConnectionsCount; Connection++)
    {
        if (!ConnectionIsOutput(StartNode->Connections[Connection])) { continue; }
        
        if (StartNode->Connections[Connection].DownstreamNodeHandle == LookForNodeHandle)
        {
            Result = true;
            break;
        }
        
        if (StartNode->Connections[Connection].DownstreamNodeHandle > 0)
        {
            interface_node* NextNode = GetNodeWithHandle(NodeList, StartNode->Connections[Connection].DownstreamNodeHandle);
            Result = CheckForRecursionWithHandle(NodeList, LookForNodeHandle, NextNode);
            if (Result) { break; }
        }
    }
    
    return Result;
}

internal b32
PortTypesMatch (interface_node* UpstreamNode, s32 UpstreamNode_OutputPort, interface_node* DownstreamNode, s32 DownstreamNode_InputPort)
{
    Assert(ConnectionIsOutput(UpstreamNode, UpstreamNode_OutputPort));
    Assert(ConnectionIsInput(DownstreamNode, DownstreamNode_InputPort));
    b32 Result = UpstreamNode->Connections[UpstreamNode_OutputPort].Type == DownstreamNode->Connections[DownstreamNode_InputPort].Type;
    return Result;
}

internal void
ConnectNodes(node_list* NodeList, 
             s32 UpstreamNodeHandle, s32 UpstreamNodePort, 
             s32 DownstreamNodeHandle, s32 DownstreamNodePort)
{
    interface_node* UpstreamNode = 0;
    interface_node* DownstreamNode = GetNodeWithHandle(NodeList, DownstreamNodeHandle);
    if (!CheckForRecursionWithHandle(NodeList, UpstreamNodeHandle, DownstreamNode))
    {
        UpstreamNode = GetNodeWithHandle(NodeList, UpstreamNodeHandle);
        if (PortTypesMatch(UpstreamNode, UpstreamNodePort, 
                           DownstreamNode, DownstreamNodePort))
        {
            Assert(ConnectionIsOutput(UpstreamNode, UpstreamNodePort));
            Assert(ConnectionIsInput(DownstreamNode, DownstreamNodePort));
            
            DownstreamNode->Connections[DownstreamNodePort].UpstreamNodeHandle = UpstreamNodeHandle;
            DownstreamNode->Connections[DownstreamNodePort].UpstreamNodePortIndex = UpstreamNodePort;
            UpstreamNode->Connections[UpstreamNodePort].DownstreamNodeHandle = DownstreamNodeHandle;
            UpstreamNode->Connections[UpstreamNodePort].DownstreamNodePortIndex = DownstreamNodePort;
        }
    }
}

internal void
UnconnectNodes (node_list* NodeList, 
                s32 DownstreamNodeHandle, s32 DownstreamNode_OutputPort, s32 UpstreamNodeHandle,  s32 UpstreamNode_InputPort)
{
    interface_node* DownstreamNode = GetNodeWithHandle(NodeList, DownstreamNodeHandle);
    interface_node* UpstreamNode = GetNodeWithHandle(NodeList, UpstreamNodeHandle);
    
    Assert(ConnectionIsOutput(DownstreamNode, DownstreamNode_OutputPort));
    Assert(ConnectionIsInput(UpstreamNode, UpstreamNode_InputPort));
    
    DownstreamNode->Connections[DownstreamNode_OutputPort].DownstreamNodeHandle = 0;
    DownstreamNode->Connections[DownstreamNode_OutputPort].DownstreamNodePortIndex = -1;
    UpstreamNode->Connections[UpstreamNode_InputPort].UpstreamNodeHandle = 0;
    UpstreamNode->Connections[UpstreamNode_InputPort].UpstreamNodePortIndex = -1;
}

internal interface_node*
GetNodeUnderPoint (node_list* NodeList, v2 Point, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    interface_node* Result = 0;
    
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        interface_node* Node = NodeIter.At;
        rect NodeBounds = CalculateNodeBounds(Node, RenderSettings);
        if (PointIsInRect(Point, NodeBounds))
        {
            Result = Node;
            break;
        }
        Next(&NodeIter);
    }
    
    return Result;
}

internal node_interaction
GetNodeInteractionType (interface_node* ActiveNode, v2 MousePos, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    node_interaction Interaction = NewNodeInteraction(ActiveNode->Handle, ActiveNode->Min - MousePos);
    
    rect NodeBounds = CalculateNodeBounds(ActiveNode, RenderSettings);
    
    
    for (s32 Connection = 0; Connection < ActiveNode->ConnectionsCount; Connection++)
    {
        // Inputs
        if (ConnectionIsInput(ActiveNode, Connection))
        {
            rect InputBounds = CalculateNodeInputPortBounds(ActiveNode, Connection, RenderSettings);
            rect ValueBounds = CalculateNodeInputValueBounds(ActiveNode, Connection, RenderSettings);
            if (PointIsInRect(MousePos, InputBounds))
            {
                Interaction.InputPort = Connection;
                Interaction.MouseOffset = MousePos - InputBounds.Min;
            }
            else if(PointIsInRect(MousePos, ValueBounds))
            {
                Interaction.InputValue = Connection;
                Interaction.MouseOffset = MousePos - ValueBounds.Min;
            }
        }
        
        // Outputs
        if (ConnectionIsOutput(ActiveNode, Connection))
        {
            rect OutputBounds = CalculateNodeOutputPortBounds(ActiveNode, Connection, RenderSettings);
            rect ValueBounds = CalculateNodeOutputValueBounds(ActiveNode, Connection, RenderSettings);
            if (PointIsInRect(MousePos, OutputBounds))
            {
                Interaction.OutputPort = Connection;
                Interaction.MouseOffset = MousePos - OutputBounds.Min;
            }
            else if(PointIsInRect(MousePos, ValueBounds))
            {
                Interaction.OutputValue = Connection;
                Interaction.MouseOffset = MousePos - ValueBounds.Min;
            }
        }
    }
    
    // Drag Handles
    rect DragUpstreamHandleBounds = CalculateNodeDragHandleBounds(NodeBounds, 0, RenderSettings);
    rect DragAllHandleBounds = CalculateNodeDragHandleBounds(NodeBounds, 1, RenderSettings);
    rect DragDownstreamHandleBounds = CalculateNodeDragHandleBounds(NodeBounds, 2, RenderSettings);
    if (PointIsInRect(MousePos, DragUpstreamHandleBounds))
    {
        Interaction.Flags = NodeInteraction_AllUpstream;
    }
    else if (PointIsInRect(MousePos, DragAllHandleBounds))
    {
        Interaction.Flags = NodeInteraction_AllUpstream | NodeInteraction_AllDownstream;
    }
    else if (PointIsInRect(MousePos, DragDownstreamHandleBounds))
    {
        Interaction.Flags = NodeInteraction_AllDownstream;
    }
    
    return Interaction;
}

internal void
TryConnectNodes (node_interaction Interaction, v2 Point, node_list* NodeList, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    if (IsDraggingNodeOutput(Interaction))
    {
        interface_node* UpstreamNode = GetNodeUnderPoint(NodeList, Point, RenderSettings);
        if (UpstreamNode)
        {
            for (s32 Connection = 0; Connection < UpstreamNode->ConnectionsCount; Connection++)
            {
                if (ConnectionIsOutput(UpstreamNode, Connection)) { continue; }
                
                rect InputBounds = CalculateNodeInputPortBounds(UpstreamNode, Connection, RenderSettings);
                if (PointIsInRect(Point, InputBounds))
                {
                    ConnectNodes(NodeList, Interaction.NodeHandle, Interaction.OutputPort,
                                 UpstreamNode->Handle, Connection);
                    break;
                }
            }
        }
    }
    else if (IsDraggingNodeInput(Interaction))
    {
        interface_node* DownstreamNode = GetNodeUnderPoint(NodeList, Point, RenderSettings);
        if (DownstreamNode)
        {
            for (s32 Connection = 0; Connection < DownstreamNode->ConnectionsCount; Connection++)
            {
                if (ConnectionIsInput(DownstreamNode, Connection)) { continue; }
                
                rect OutputBounds = CalculateNodeOutputPortBounds(DownstreamNode, Connection, RenderSettings);
                if (PointIsInRect(Point, OutputBounds))
                {
                    ConnectNodes(NodeList, 
                                 DownstreamNode->Handle, Connection,
                                 Interaction.NodeHandle, Interaction.InputPort);
                    break;
                }
            }
        }
    }
}

internal void
PlaceNode (node_list* NodeList, interface_node* Node, v2 Position, b32 Flags)
{
    DEBUG_TRACK_FUNCTION;
    
    v2 Offset = Position - Node->Min;
    
    if (Flags & NodeInteraction_AllDownstream)
    {
        for (s32 Connection = 0; Connection < Node->ConnectionsCount; Connection++)
        {
            if (!ConnectionIsOutput(Node, Connection)) { continue; }
            
            s32 ConnectionHandle = Node->Connections[Connection].DownstreamNodeHandle;
            if (ConnectionHandle > 0)
            {
                interface_node* ConnectedNode = GetNodeWithHandle(NodeList, ConnectionHandle);
                v2 CurrPos = ConnectedNode->Min;
                v2 NewPos = CurrPos + Offset;
                // NOTE(Peter): Have to negate the all downstream component so it doesn't turn around and try
                // to move this node again.
                PlaceNode(NodeList, ConnectedNode, NewPos, Flags & ~NodeInteraction_AllUpstream);
            }
        }
    }
    
    if (Flags & NodeInteraction_AllUpstream)
    {
        for (s32 Connection = 0; Connection < Node->ConnectionsCount; Connection++)
        {
            if (!ConnectionIsInput(Node, Connection)) { continue; }
            
            s32 ConnectionHandle = Node->Connections[Connection].UpstreamNodeHandle;
            if (ConnectionHandle > 0)
            {
                interface_node* ConnectedNode = GetNodeWithHandle(NodeList, ConnectionHandle);
                v2 CurrPos = ConnectedNode->Min;
                v2 NewPos = CurrPos + Offset;
                // NOTE(Peter): Have to negate the all upstream component so it doesn't turn around and try
                // to move this node again.
                PlaceNode(NodeList, ConnectedNode, NewPos, Flags & ~NodeInteraction_AllDownstream);
            }
        }
    }
    
    Node->Min = Position;
}

internal void
UpdateDraggingNode (v2 MousePos, node_interaction Interaction, node_list* NodeList, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    if (IsDraggingNode(Interaction))
    {
        interface_node* ActiveNode = GetNodeWithHandle(NodeList, Interaction.NodeHandle);
        PlaceNode(NodeList, ActiveNode, MousePos + Interaction.MouseOffset, Interaction.Flags);
    }
}

internal void
UpdateDraggingNodePort (v2 MousePos, node_interaction Interaction, node_list* NodeList, node_render_settings RenderSettings, render_command_buffer* RenderBuffer)
{
    DEBUG_TRACK_FUNCTION;
    
    if (IsDraggingNodePort(Interaction))
    {
        interface_node* ActiveNode = GetNodeWithHandle(NodeList, Interaction.NodeHandle);
        rect PortBounds = {};
        if (IsDraggingNodeInput(Interaction))
        {
            PortBounds = CalculateNodeInputPortBounds(ActiveNode, Interaction.InputPort, RenderSettings);
        }
        else if (IsDraggingNodeOutput(Interaction))
        {
            PortBounds = CalculateNodeOutputPortBounds(ActiveNode, Interaction.OutputPort, RenderSettings);
        }
        
        v2 PortCenter = CalculateRectCenter(PortBounds);
        PushRenderLine2D(RenderBuffer, PortCenter, MousePos, 1, WhiteV4);
    }
}

internal void
UpdateDraggingNodeValue (v2 MousePos, v2 LastFrameMousePos, node_interaction Interaction, node_list* NodeList, node_render_settings RenderSettings, app_state* State)
{
    DEBUG_TRACK_FUNCTION;
    
    if(IsDraggingNodeValue(Interaction))
    {
        v2 MouseDelta = MousePos - LastFrameMousePos;
        interface_node* Node = GetNodeWithHandle(NodeList, Interaction.NodeHandle);
        
        node_connection* Connection = 0;
        if (IsDraggingNodeInputValue(Interaction))
        {
            Connection = Node->Connections + Interaction.InputValue;
            Assert(ConnectionIsInput(*Connection));
        }
        else if (IsDraggingNodeOutputValue(Interaction))
        {
            Connection = Node->Connections + Interaction.OutputValue;
            Assert(ConnectionIsOutput(*Connection));
        }
        Assert(Connection);
        
        switch (Connection->Type)
        {
            case MemberType_s32:
            {
                Connection->S32Value += (s32)(MouseDelta.y * .05f);
            }break;
            
            case MemberType_r32:
            {
                Connection->R32Value += (MouseDelta.y * .05f);
            }break;
            
            case MemberType_v4:
            {
            }break;
            
            case MemberType_NODE_COLOR_BUFFER: {} break; // NOTE(Peter): Unused for now
            InvalidDefaultCase;
        }
    }
}


internal void UpdateNodeCalculation (interface_node* Node, node_list* NodeList, 
                                     memory_arena* Permanent, memory_arena* Transient,
                                     led* LEDs, sacn_pixel* ColorsInit, s32 LEDCount, r32 DeltaTime);

internal void
UpdateNodesConnectedUpstream (interface_node* Node, node_list* NodeList, 
                              memory_arena* Permanent, memory_arena* Transient,
                              led* LEDs, sacn_pixel* ColorsInit, s32 LEDCount, r32 DeltaTime)
{
    for (s32 ConnectionIdx = 0; ConnectionIdx < Node->ConnectionsCount; ConnectionIdx++)
    {
        node_connection* Connection = 0;
        if (ConnectionIsInput(Node->Connections[ConnectionIdx]))
        {
            Connection = Node->Connections + ConnectionIdx;
            
            if (ConnectionHasUpstreamConnection(*Connection))
            {
                interface_node* UpstreamNode = GetNodeWithHandle(NodeList, Connection->UpstreamNodeHandle);
                if (!UpstreamNode->UpdatedThisFrame)
                {
                    UpdateNodeCalculation(UpstreamNode, NodeList, Permanent, Transient, LEDs, ColorsInit, LEDCount, DeltaTime);
                }
                switch (Connection->Type)
                {
                    case MemberType_s32:
                    {
                        Connection->S32Value = UpstreamNode->Connections[Connection->UpstreamNodePortIndex].S32Value;
                    }break;
                    
                    case MemberType_r32:
                    {
                        Connection->R32Value = UpstreamNode->Connections[Connection->UpstreamNodePortIndex].R32Value;
                    }break;
                    
                    case MemberType_v4:
                    {
                        Connection->V4Value = UpstreamNode->Connections[Connection->UpstreamNodePortIndex].V4Value;
                    }break;
                    
                    case MemberType_NODE_COLOR_BUFFER:
                    {
                        Connection->LEDsValue = UpstreamNode->Connections[Connection->UpstreamNodePortIndex].LEDsValue;
                    }break;
                    
                    InvalidDefaultCase;
                }
            }
        }
    }
}

internal void
UpdateNodeCalculation (interface_node* Node, node_list* NodeList, 
                       memory_arena* Permanent, memory_arena* Transient,
                       led* LEDs, sacn_pixel* ColorsInit, s32 LEDCount, r32 DeltaTime)
{
    DEBUG_TRACK_FUNCTION;
    Assert(Node->PersistentData != 0);
    
    // NOTE(Peter): Have to subtract one here so that we account for the 
    // NodeType_OutputNode entry in the enum
    node_specification Spec = NodeSpecifications[Node->Type - 1];
    node_struct_member* MemberList = Spec.MemberList;
    
    // NOTE(Peter): We do this at the beginning in case there is a node connected to this one
    // which has a connection that is both an Input and Output. In that case, if UpdatedThisFrame
    // were not set before hand, the two nodes would pingpong back and forth trying to get 
    // eachother to update.
    Node->UpdatedThisFrame = true;
    
    sacn_pixel* Colors = ColorsInit;
    
    UpdateNodesConnectedUpstream(Node, NodeList, Permanent, Transient, LEDs, Colors, LEDCount, DeltaTime);
    
    for (s32 ConnectionIdx = 0; ConnectionIdx < Node->ConnectionsCount; ConnectionIdx++)
    {
        node_connection Connection = Node->Connections[ConnectionIdx];
        
        // TODO(Peter): We're currently passing in a pointer to the leds array for every single
        // NODE_COLOR_BUFFER. We shouldn't do that, and just require each data structure that
        // needs the leds to request that as its own member/parameter.
        if (Connection.Type == MemberType_NODE_COLOR_BUFFER)
        {
            node_led_color_connection* ColorConnection = (node_led_color_connection*)(Node->PersistentData + MemberList[ConnectionIdx].Offset);
            
            ColorConnection->LEDs = LEDs;
            ColorConnection->LEDCount = LEDCount;
            ColorConnection->Colors = Connection.LEDsValue.Colors;
            
            if (!ColorConnection->Colors)
            {
                sacn_pixel* ColorsCopy = PushArray(Transient, sacn_pixel, LEDCount);
                GSMemSet((u8*)ColorsCopy, 0, sizeof(sacn_pixel) * LEDCount);
                ColorConnection->Colors = ColorsCopy;
            }
        }
        else if (ConnectionIsInput(Connection))
        {
            Assert(Connection.Type != MemberType_NODE_COLOR_BUFFER);
            switch (Connection.Type)
            {
                case MemberType_s32:
                {
                    GSMemCopy(&Connection.S32Value, (Node->PersistentData + MemberList[ConnectionIdx].Offset), sizeof(s32));
                }break;
                
                case MemberType_r32:
                {
                    GSMemCopy(&Connection.R32Value, (Node->PersistentData + MemberList[ConnectionIdx].Offset), sizeof(r32));
                }break;
                
                case MemberType_v4:
                {
                    GSMemCopy(&Connection.V4Value, (Node->PersistentData + MemberList[ConnectionIdx].Offset), sizeof(v4));
                }break;
                
                InvalidDefaultCase;
            }
        }
    }
    
    CallNodeProc(Node, Node->PersistentData, LEDs, LEDCount, DeltaTime);
    
    for (s32 ConnectionIdx = 0; ConnectionIdx < Node->ConnectionsCount; ConnectionIdx++)
    {
        node_connection* Connection = 0;
        if (ConnectionIsOutput(Node, ConnectionIdx))
        {
            Connection = Node->Connections + ConnectionIdx;
        }
        else
        {
            continue;
        }
        
        switch (Connection->Type)
        {
            case MemberType_s32:
            {
                GSMemCopy((Node->PersistentData + MemberList[ConnectionIdx].Offset), &Connection->S32Value, sizeof(s32));
            }break;
            
            case MemberType_r32:
            {
                GSMemCopy((Node->PersistentData + MemberList[ConnectionIdx].Offset), &Connection->R32Value, sizeof(r32));
            }break;
            
            case MemberType_v4:
            {
                GSMemCopy((Node->PersistentData + MemberList[ConnectionIdx].Offset), &Connection->V4Value, sizeof(v4));
            }break;
            
            case MemberType_NODE_COLOR_BUFFER:
            {
                node_led_color_connection* Value = (node_led_color_connection*)(Node->PersistentData + MemberList[ConnectionIdx].Offset);
                Connection->LEDsValue.Colors = Value->Colors;
            }break;
            
            InvalidDefaultCase;
        }
    }
}

internal void
UpdateOutputNodeCalculations (interface_node* OutputNode, node_list* NodeList, 
                              memory_arena* Permanent, memory_arena* Transient, 
                              led* LEDs, sacn_pixel* Colors, s32 LEDCount, r32 DeltaTime)
{
    Assert(OutputNode->Type == NodeType_OutputNode);
    UpdateNodesConnectedUpstream(OutputNode, NodeList, Permanent, Transient, LEDs, Colors, LEDCount, DeltaTime);
    
    node_connection ColorsConnection = OutputNode->Connections[0];
    if (ColorsConnection.LEDsValue.Colors)
    {
        sacn_pixel* DestPixel = Colors;
        sacn_pixel* SourcePixel = ColorsConnection.LEDsValue.Colors;
        for (s32 i = 0; i < LEDCount; i++)
        {
            *DestPixel++ = *SourcePixel++;
        }
    }
}

internal void
UpdateAllNodeCalculations (node_list* NodeList, memory_arena* Permanent, memory_arena* Transient, led* LEDs, sacn_pixel* Colors, s32 LEDCount, r32 DeltaTime)
{
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        interface_node* Node = NodeIter.At;
        if (!Node->UpdatedThisFrame)
        {
            UpdateNodeCalculation(Node, NodeList, Permanent, Transient, LEDs, Colors, LEDCount, DeltaTime);
        }
        Next(&NodeIter);
    }
}

internal void
ClearTransientNodeColorBuffers (node_list* NodeList)
{
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        interface_node* Node = NodeIter.At;
        for (s32 ConnectionIdx = 0; ConnectionIdx < Node->ConnectionsCount; ConnectionIdx++)
        {
            node_connection* Connection = Node->Connections + ConnectionIdx;
            if (Connection->Type == MemberType_NODE_COLOR_BUFFER)
            {
                Connection->LEDsValue.Colors = 0;
            }
        }
        Next(&NodeIter);
    }
}

internal void
DrawValueDisplay (render_command_buffer* RenderBuffer, rect Bounds, node_connection Value, bitmap_font* Font)
{
    PushRenderQuad2D(RenderBuffer, Bounds.Min, Bounds.Max, BlackV4);
    
    char Buffer[32];
    string String = MakeString(Buffer, 32);
    
    switch (Value.Type)
    {
        case MemberType_s32:
        {
            PrintF(&String, "%.*d", 4, Value.S32Value);
            DrawString(RenderBuffer, String, Font, Bounds.Min + v2{2, 2}, WhiteV4);
        }break;
        
        case MemberType_r32:
        {
            PrintF(&String, "%.*f", 4, Value.R32Value);
            DrawString(RenderBuffer, String, Font, Bounds.Min + v2{2, 2}, WhiteV4);
        }break;
        
        case MemberType_v4:
        {
            PushRenderQuad2D(RenderBuffer, Bounds.Min + v2{2, 2}, Bounds.Max - v2{2, 2}, Value.V4Value);
        }break;
        
        case MemberType_NODE_COLOR_BUFFER:
        {
            PrintF(&String, "LEDs");
            DrawString(RenderBuffer, String, Font, Bounds.Min +  v2{2, 2}, WhiteV4);
        }break;
        
        InvalidDefaultCase;
    }
}

internal void
DrawPort (render_command_buffer* RenderBuffer, rect Bounds, v4 Color)
{
    PushRenderQuad2D(RenderBuffer, Bounds.Min, Bounds.Max, Color);
}

internal void
ResetNodesUpdateState (node_list* NodeList)
{
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        interface_node* Node = NodeIter.At;
        Node->UpdatedThisFrame = false;
        Next(&NodeIter);
    }
}

internal b32 
SpecificationPassesFilter(string SpecificationName, string SearchString)
{
    return (SearchString.Length == 0 || StringContainsStringCaseInsensitive(SpecificationName, SearchString));
}

internal s32
NodeListerConvertHotItemToListIndex (s32 HotItem, u8* NodeSpecificationsList, s32 NodeSpecificationsListCount,
                                     string SearchString)
{
    s32 ListIndex = 0;
    s32 FilteredItemsCount = 0;
    
    for (s32 i = 0; i < NodeSpecificationsListCount; i++)
    {
        node_specification* Specification = (node_specification*)NodeSpecificationsList + i;
        string ItemName = MakeString(Specification->Name);
        b32 PassesFilter = SpecificationPassesFilter(ItemName, SearchString);
        if (PassesFilter)
        {
            if (FilteredItemsCount == HotItem)
            {
                break;
            }
            FilteredItemsCount++;
        }
        ListIndex++;
    }
    
    return ListIndex;
}

internal string
NodeListerGetNodeName (u8* NodeSpecificationsList, s32 NodeSpecificationsListCount, string SearchString, s32 Offset)
{
    s32 FilteredItemsCount = 0;
    node_specification* Specification = (node_specification*)NodeSpecificationsList;
    string Result = {};
    for (s32 i = 0; i < NodeSpecificationsListCount; i++)
    {
        string ItemName = MakeString(Specification->Name);
        b32 PassesFilter = SpecificationPassesFilter(ItemName, SearchString);
        if (PassesFilter)
        {
            if (FilteredItemsCount == Offset)
            {
                Result = ItemName;
                break;
            }
            FilteredItemsCount++;
        }
        
        Specification++;
    }
    
    return Result;
}
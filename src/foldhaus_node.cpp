
inline s32
GetNodeMemorySize (interface_node Node)
{
    s32 Result = sizeof(interface_node) + (sizeof(node_connection) * Node.ConnectionsCount) + sizeof(Node.Name);
    return Result;
}

internal node_list_iterator
GetNodeListIterator(node_list List)
{
    node_list_iterator Result = {};
    Result.List = List;
    Result.At = (interface_node*)List.Memory;
    return Result;
}

internal b32
NodeIteratorIsValid(node_list_iterator Iter)
{
    b32 Result = (Iter.At != 0); 
    Result &= (((u8*)Iter.At - Iter.List.Memory) < Iter.List.Used);
    return Result;
}

internal s32
GetCurrentOffset (node_list_iterator Iter)
{
    s32 Result = (u8*)Iter.At - Iter.List.Memory;
    return Result;
}

internal void
Next (node_list_iterator* Iter)
{
    s32 SkipAmount = GetNodeMemorySize(*Iter->At);
    if (((u8*)Iter->At - Iter->List.Memory) + SkipAmount < Iter->List.Used)
    {
        Iter->At = (interface_node*)((u8*)Iter->At + SkipAmount);
    }
    else if (Iter->List.Next)
    {
        Iter->List = *Iter->List.Next;
        Iter->At = (interface_node*)Iter->List.Memory;
    }
    else
    {
        Iter->At = 0;
    }
}

internal node_list*
AllocateNodeList (memory_arena* Storage, s32 Size)
{
    node_list* Result = PushStruct(Storage, node_list);
    Result->Memory = PushSize(Storage, Size);
    Result->Max = Size;
    Result->Used = 0;
    Result->Next = 0;
    return Result;
}

internal interface_node*
PushNodeOnList (node_list* List, s32 NameLength, s32 ConnectionsCount, v2 Min, v2 Dim, memory_arena* Storage)
{
    interface_node* Result = 0;
    
    if (List->Used >= List->Max)
    {
        if (!List->Next)
        {
            List->Next = AllocateNodeList(Storage, List->Max);
        }
        Result = PushNodeOnList(List->Next, NameLength, ConnectionsCount, Min, Dim, Storage);
    }
    else
    {
        Result = (interface_node*)(List->Memory + List->Used);
        Result->Name = MakeString((char*)(Result + 1), NameLength);
        
        Result->ConnectionsCount = ConnectionsCount;
        Result->Connections = (node_connection*)(Result->Name.Memory + NameLength);
        
        Result->Min = Min;
        Result->MinAfterUpdate = Min;
        Result->Dim = Dim;
        
        List->Used += GetNodeMemorySize(*Result);
    }
    
    return Result;
}

internal void
InitializeNodeConnection (node_connection* Connection, struct_member_type Type, b32 DirectionMask)
{
    Connection->Type = Type;
    Connection->UpstreamNodeOffset = -1;
    Connection->UpstreamNodePortIndex = -1;
    Connection->DownstreamNodeOffset = -1;
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
GetNodeAtOffset (node_list* List, s32 Offset)
{
    DEBUG_TRACK_FUNCTION;
    
    interface_node* Node = 0;
    if (Offset <= List->Used)
    {
        Node = (interface_node*)(List->Memory + Offset);
    }
    else if (List->Next)
    {
        Node = GetNodeAtOffset(List->Next, Offset - List->Max);
    }
    return Node;
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
    interface_node* ConnectedNode = GetNodeAtOffset(NodeList, Node->Connections[PortIndex].UpstreamNodeOffset);
    rect Result = CalculateNodeOutputPortBounds(ConnectedNode, Node->Connections[PortIndex].UpstreamNodePortIndex, RenderSettings);
    return Result;
}

internal rect
GetBoundsOfPortConnectedToOutput (interface_node* Node, s32 PortIndex, node_list* NodeList, node_render_settings RenderSettings)
{
    interface_node* ConnectedNode = GetNodeAtOffset(NodeList, Node->Connections[PortIndex].DownstreamNodeOffset);
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
    Result.NodeOffset = -1;
    Result.InputPort = -1;
    Result.InputValue = -1;
    Result.OutputPort = -1;
    Result.OutputValue = -1;
    return Result;
}

internal node_interaction
NewNodeInteraction (s32 NodeOffset, v2 MouseOffset)
{
    node_interaction Result = {};
    Result.NodeOffset = NodeOffset;
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
    b32 Result = ((Interaction.NodeOffset >= 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0) && 
                  (Interaction.OutputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodePort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeOffset >= 0) &&
                  (Interaction.InputPort >= 0 || Interaction.OutputPort >= 0) &&
                  (Interaction.InputValue < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeOffset >= 0) &&
                  (Interaction.InputPort < 0 && Interaction.OutputPort < 0) &&
                  (Interaction.InputValue >= 0 || Interaction.OutputValue >= 0));
    return Result;
}

internal b32
IsDraggingNodeInput (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeOffset >= 0) && 
                  (Interaction.InputPort >= 0 || Interaction.InputValue >= 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeInputPort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeOffset >= 0) && 
                  (Interaction.InputPort >= 0) && (Interaction.InputValue < 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeInputValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeOffset >= 0) && 
                  (Interaction.InputPort < 0) &&  (Interaction.InputValue >= 0) && 
                  (Interaction.OutputPort < 0 && Interaction.OutputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutput (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeOffset >= 0) && 
                  (Interaction.OutputPort >= 0 || Interaction.OutputValue >= 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutputPort (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeOffset >= 0) && 
                  (Interaction.OutputPort >= 0) && (Interaction.OutputValue < 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
IsDraggingNodeOutputValue (node_interaction Interaction)
{
    b32 Result = ((Interaction.NodeOffset >= 0) && 
                  (Interaction.OutputPort < 0) &&  (Interaction.OutputValue >= 0) && 
                  (Interaction.InputPort < 0 && Interaction.InputValue < 0));
    return Result;
}

internal b32
ConnectionIsConnected (node_connection Connection)
{
    b32 Result = (Connection.UpstreamNodeOffset >= 0) || (Connection.DownstreamNodeOffset >= 0);
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
    b32 Result = (Connection.UpstreamNodeOffset >= 0);
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
    b32 Result = (Connection.DownstreamNodeOffset >= 0);
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
CheckForRecursion (node_list* NodeList, s32 LookForNode, interface_node* StartNode)
{
    DEBUG_TRACK_FUNCTION;
    b32 Result = false;
    
    for (s32 Connection = 0; Connection < StartNode->ConnectionsCount; Connection++)
    {
        if (!ConnectionIsOutput(StartNode->Connections[Connection])) { continue; }
        
        if (StartNode->Connections[Connection].DownstreamNodeOffset == LookForNode)
        {
            Result = true;
            break;
        }
        
        if (StartNode->Connections[Connection].DownstreamNodeOffset >= 0)
        {
            interface_node* NextNode = GetNodeAtOffset(NodeList, StartNode->Connections[Connection].DownstreamNodeOffset);
            Result = CheckForRecursion(NodeList, LookForNode, NextNode);
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
             s32 UpstreamNodeOffset, s32 UpstreamNodePort, 
             s32 DownstreamNodeOffset, s32 DownstreamNodePort)
{
    interface_node* DownstreamNode = GetNodeAtOffset(NodeList, DownstreamNodeOffset);
    if (!CheckForRecursion(NodeList, UpstreamNodeOffset, DownstreamNode))
    {
        interface_node* UpstreamNode = GetNodeAtOffset(NodeList, UpstreamNodeOffset);
        if (PortTypesMatch(UpstreamNode, UpstreamNodePort, 
                           DownstreamNode, DownstreamNodePort))
        {
            Assert(ConnectionIsOutput(UpstreamNode, UpstreamNodePort));
            Assert(ConnectionIsInput(DownstreamNode, DownstreamNodePort));
            
            DownstreamNode->Connections[DownstreamNodePort].UpstreamNodeOffset = UpstreamNodeOffset;
            DownstreamNode->Connections[DownstreamNodePort].UpstreamNodePortIndex = UpstreamNodePort;
            UpstreamNode->Connections[UpstreamNodePort].DownstreamNodeOffset = DownstreamNodeOffset;
            UpstreamNode->Connections[UpstreamNodePort].DownstreamNodePortIndex = DownstreamNodePort;
        }
    }
}

internal void
UnconnectNodes (node_list* NodeList, s32 DownstreamNodeOffset, s32 DownstreamNode_OutputPort, s32 UpstreamNodeOffset, s32 UpstreamNode_InputPort)
{
    interface_node* DownstreamNode = GetNodeAtOffset(NodeList, DownstreamNodeOffset);
    interface_node* UpstreamNode = GetNodeAtOffset(NodeList, UpstreamNodeOffset);
    
    Assert(ConnectionIsOutput(DownstreamNode, DownstreamNode_OutputPort));
    Assert(ConnectionIsInput(UpstreamNode, UpstreamNode_InputPort));
    
    DownstreamNode->Connections[DownstreamNode_OutputPort].DownstreamNodeOffset = -1;
    DownstreamNode->Connections[DownstreamNode_OutputPort].DownstreamNodePortIndex = -1;
    UpstreamNode->Connections[UpstreamNode_InputPort].UpstreamNodeOffset = -1;
    UpstreamNode->Connections[UpstreamNode_InputPort].UpstreamNodePortIndex = -1;
}

internal node_offset
GetNodeUnderPoint (node_list* NodeList, v2 Point, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    node_offset Result = {0, -1};
    
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        interface_node* Node = NodeIter.At;
        rect NodeBounds = CalculateNodeBounds(Node, RenderSettings);
        if (PointIsInRect(Point, NodeBounds))
        {
            Result.Node = Node;
            Result.Offset = (s32)((u8*)NodeIter.At - NodeList->Memory);
            break;
        }
        Next(&NodeIter);
    }
    
    return Result;
}

internal node_interaction
GetNodeInteractionType (interface_node* ActiveNode, s32 NodeOffset, v2 MousePos, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    node_interaction Interaction = NewNodeInteraction(NodeOffset, ActiveNode->Min - MousePos);
    
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
        node_offset UpstreamNodeOffset = GetNodeUnderPoint(NodeList, Point, RenderSettings);
        if (UpstreamNodeOffset.Node)
        {
            for (s32 Connection = 0; Connection < UpstreamNodeOffset.Node->ConnectionsCount; Connection++)
            {
                if (ConnectionIsOutput(UpstreamNodeOffset.Node, Connection)) { continue; }
                
                rect InputBounds = CalculateNodeInputPortBounds(UpstreamNodeOffset.Node, Connection, RenderSettings);
                if (PointIsInRect(Point, InputBounds))
                {
                    ConnectNodes(NodeList, Interaction.NodeOffset, Interaction.OutputPort,
                                 UpstreamNodeOffset.Offset, Connection);
                    break;
                }
            }
        }
    }
    else if (IsDraggingNodeInput(Interaction))
    {
        node_offset DownstreamNodeOffset = GetNodeUnderPoint(NodeList, Point, RenderSettings);
        if (DownstreamNodeOffset.Node)
        {
            for (s32 Connection = 0; Connection < DownstreamNodeOffset.Node->ConnectionsCount; Connection++)
            {
                if (ConnectionIsInput(DownstreamNodeOffset.Node, Connection)) { continue; }
                
                rect OutputBounds = CalculateNodeOutputPortBounds(DownstreamNodeOffset.Node, Connection, RenderSettings);
                if (PointIsInRect(Point, OutputBounds))
                {
                    ConnectNodes(NodeList, 
                                 DownstreamNodeOffset.Offset, Connection,
                                 Interaction.NodeOffset, Interaction.InputPort);
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
            
            s32 ConnectionOffset = Node->Connections[Connection].DownstreamNodeOffset;
            if (ConnectionOffset >= 0)
            {
                interface_node* ConnectedNode = GetNodeAtOffset(NodeList, ConnectionOffset);
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
            
            s32 ConnectionOffset = Node->Connections[Connection].UpstreamNodeOffset;
            if (ConnectionOffset >= 0)
            {
                interface_node* ConnectedNode = GetNodeAtOffset(NodeList, ConnectionOffset);
                v2 CurrPos = ConnectedNode->Min;
                v2 NewPos = CurrPos + Offset;
                // NOTE(Peter): Have to negate the all upstream component so it doesn't turn around and try
                // to move this node again.
                PlaceNode(NodeList, ConnectedNode, NewPos, Flags & ~NodeInteraction_AllDownstream);
            }
        }
    }
    
    Node->MinAfterUpdate = Position;
}

internal void
UpdateDraggingNode (v2 MousePos, node_interaction Interaction, node_list* NodeList, node_render_settings RenderSettings)
{
    DEBUG_TRACK_FUNCTION;
    
    if (IsDraggingNode(Interaction))
    {
        interface_node* ActiveNode = GetNodeAtOffset(NodeList, Interaction.NodeOffset);
        PlaceNode(NodeList, ActiveNode, MousePos + Interaction.MouseOffset, Interaction.Flags);
    }
}

internal void
UpdateDraggingNodePort (v2 MousePos, node_interaction Interaction, node_list* NodeList, node_render_settings RenderSettings, render_command_buffer* RenderBuffer)
{
    DEBUG_TRACK_FUNCTION;
    
    if (IsDraggingNodePort(Interaction))
    {
        interface_node* ActiveNode = GetNodeAtOffset(NodeList, Interaction.NodeOffset);
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
        interface_node* Node = GetNodeAtOffset(NodeList, Interaction.NodeOffset);
        
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
                // TODO(Peter): This is a problem. We seem to be calling this several times
                if (MagSqr(MouseDelta) < 10)
                {
                    OpenColorPicker(State, &Connection->V4Value);
                }
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
                interface_node* UpstreamNode = GetNodeAtOffset(NodeList, Connection->UpstreamNodeOffset);
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
            DrawString(RenderBuffer, String, Font, Font->PixelHeight, Bounds.Min + v2{2, 2}, WhiteV4);
        }break;
        
        case MemberType_r32:
        {
            PrintF(&String, "%.*f", 4, Value.R32Value);
            DrawString(RenderBuffer, String, Font, Font->PixelHeight, Bounds.Min + v2{2, 2}, WhiteV4);
        }break;
        
        case MemberType_v4:
        {
            PushRenderQuad2D(RenderBuffer, Bounds.Min + v2{2, 2}, Bounds.Max - v2{2, 2}, Value.V4Value);
        }break;
        
        case MemberType_NODE_COLOR_BUFFER:
        {
            PrintF(&String, "LEDs");
            DrawString(RenderBuffer, String, Font, Font->PixelHeight, Bounds.Min +  v2{2, 2}, WhiteV4);
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
RenderNodeList (node_list* NodeList, node_render_settings RenderSettings, render_command_buffer* RenderBuffer)
{
    DEBUG_TRACK_FUNCTION;
    
    node_list_iterator NodeIter = GetNodeListIterator(*NodeList);
    while (NodeIteratorIsValid(NodeIter))
    {
        interface_node* Node = NodeIter.At;
        Node->Min = Node->MinAfterUpdate;
        
        rect NodeBounds = CalculateNodeBounds(Node, RenderSettings);
        
        PushRenderQuad2D(RenderBuffer, NodeBounds.Min, NodeBounds.Max, v4{.5f, .5f, .5f, 1.f});
        
        DrawString(RenderBuffer, Node->Name, RenderSettings.Font, RenderSettings.Font->PixelHeight,
                   v2{NodeBounds.Min.x + 5, NodeBounds.Max.y - (RenderSettings.Font->PixelHeight + NODE_HEADER_HEIGHT + 5)},
                   WhiteV4);
        
        for (s32 Connection = 0; Connection < Node->ConnectionsCount; Connection++)
        {
            // Inputs
            if (ConnectionIsInput(Node, Connection))
            {
                rect PortBounds = CalculateNodeInputPortBounds(Node, Connection, RenderSettings);
                v4 PortColor = RenderSettings.PortColors[Node->Connections[Connection].Type];
                DrawPort(RenderBuffer, PortBounds, PortColor);
                
                rect ValueBounds = CalculateNodeInputValueBounds(Node, Connection, RenderSettings);
                DrawValueDisplay(RenderBuffer, ValueBounds, Node->Connections[Connection], RenderSettings.Font);
                
                // NOTE(Peter): its way easier to draw the connection on the input port b/c its a 1:1 relationship,
                // whereas output ports might have many connections, they really only know about the most recent one
                // Not sure if this is a problem. We mostly do everything backwards here, starting at the 
                // most downstream node and working back up to find dependencies.
                if (ConnectionHasUpstreamConnection(Node, Connection))
                {
                    rect ConnectedPortBounds = GetBoundsOfPortConnectedToInput(Node, Connection, NodeList, RenderSettings);
                    v2 InputCenter = CalculateRectCenter(PortBounds);
                    v2 OutputCenter = CalculateRectCenter(ConnectedPortBounds);
                    PushRenderLine2D(RenderBuffer, OutputCenter, InputCenter, 1, WhiteV4);
                }
            }
            
            // Outputs
            if (ConnectionIsOutput(Node, Connection))
            {
                rect PortBounds = CalculateNodeOutputPortBounds(Node, Connection, RenderSettings);
                v4 PortColor = RenderSettings.PortColors[Node->Connections[Connection].Type];
                DrawPort(RenderBuffer, PortBounds, PortColor);
                
                rect ValueBounds = CalculateNodeOutputValueBounds(Node, Connection, RenderSettings);
                DrawValueDisplay(RenderBuffer, ValueBounds, Node->Connections[Connection], RenderSettings.Font);
            }
            
            for (s32 Button = 0; Button < 3; Button++)
            {
                rect ButtonRect = CalculateNodeDragHandleBounds(NodeBounds, Button, RenderSettings);
                PushRenderQuad2D(RenderBuffer, ButtonRect.Min, ButtonRect.Max, DragButtonColors[Button]);
            }
        }
        
        Next(&NodeIter);
    }
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
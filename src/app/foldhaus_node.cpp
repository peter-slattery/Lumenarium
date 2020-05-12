//
// File: foldhaus_node.cpp
// Author: Peter Slattery
// Creation Date: 2020-01-01
//
#ifndef FOLDHAUS_NODE_CPP

internal b32
MemberIsInput(gsm_struct_member_type_info Member)
{
    b32 Result = (0 <= gsm_GetMetaTagIndex(MetaTag_node_input, Member.Tags, Member.TagsCount));
    return Result;
}

internal b32
MemberIsOutput(gsm_struct_member_type_info Member)
{
    b32 Result = (0 <= gsm_GetMetaTagIndex(MetaTag_node_output, Member.Tags, Member.TagsCount));
    return Result;
}

internal void
ClearNodeWorkspaceStorage(pattern_node_workspace* Workspace)
{
    ClearArena(&Workspace->Storage);
    Workspace->SparseToSortedNodeMap = 0;
    Workspace->SortedNodeHandles = 0;
}

struct adjacency_list
{
    // TODO(Peter): Can make these as buffers, not single digits later
    gs_list_handle NodeHandle;
    adjacency_list* Next;
};

internal u32
SortNodeNeighbors(u32 ContiguousNodeIndex, gs_list_handle NodeHandle, adjacency_list** NeighborsLists, b8* NodesVisited, gs_list_handle* SortedNodeHandles, u32 SortedNodesCount, s32* SparseToContiguousNodeMap)
{
    NodesVisited[ContiguousNodeIndex] = true;
    
    adjacency_list* Neighbor = NeighborsLists[ContiguousNodeIndex];
    while (Neighbor)
    {
        u32 ContiguousNeighborNodeIndex = SparseToContiguousNodeMap[Neighbor->NodeHandle.Index];
        if (!NodesVisited[ContiguousNeighborNodeIndex])
        {
            SortedNodesCount = SortNodeNeighbors(ContiguousNeighborNodeIndex, Neighbor->NodeHandle, NeighborsLists, NodesVisited, SortedNodeHandles, SortedNodesCount, SparseToContiguousNodeMap); 
        }
        Neighbor = Neighbor->Next;
    }
    
    SortedNodeHandles[SortedNodesCount++] = NodeHandle;
    return SortedNodesCount;
}

internal s32*
CreateSparseToContiguousMap (pattern_node_workspace Workspace, memory_arena* Scratch)
{
    s32* Result = PushArray(Scratch, s32, Workspace.Nodes.OnePastLastUsed);
    s32 ContiguousIndex = 0;
    for (u32 SparseNodeIndex = 0; SparseNodeIndex < Workspace.Nodes.OnePastLastUsed; SparseNodeIndex++)
    {
        gs_list_entry<pattern_node>* Entry = Workspace.Nodes.GetEntryAtIndex(SparseNodeIndex);
        if (!EntryIsFree(Entry))
        {
            Result[SparseNodeIndex] = ContiguousIndex++;
        }
    }
    return Result;
}

internal void
UpdateSortedNodes(pattern_node_workspace* Workspace, memory_arena* Scratch)
{
    ClearNodeWorkspaceStorage(Workspace);
    
    u32 NodeCount = Workspace->Nodes.Used;
    u32 SparseNodeCount = Workspace->Nodes.OnePastLastUsed;
    
    s32* SparseToContiguousNodeMap = CreateSparseToContiguousMap(*Workspace, &Workspace->Storage);
    
    // NOTE(Peter): We need to sort this later on so I'm just storing list lengths in this format
    // to begin with. 
    // NeighborsListLengths[n].Radix = the number of neighbors for the node
    // NeighborsListLengths[n].ID = the sparse array index of the node
    gs_radix_entry* NeighborsListLengths = PushArray(Scratch, gs_radix_entry, NodeCount);
    adjacency_list** NeighborsLists = PushArray(Scratch, adjacency_list*, NodeCount);
    GSZeroArray(NeighborsLists, adjacency_list*, SparseNodeCount);
    
    // Fill Radix
    for (u32 n = 0; n < SparseNodeCount; n++)
    {
        s32 ContiguousIndex = SparseToContiguousNodeMap[n];
        if (ContiguousIndex >= 0)
        {
            NeighborsListLengths[ContiguousIndex].Radix = 0;
            NeighborsListLengths[ContiguousIndex].ID = n;
        }
    }
    
    // Construct Adjaceny List
    for (u32 c = 0; c < Workspace->Connections.Used; c++)
    {
        pattern_node_connection Connection = *Workspace->Connections.GetElementAtIndex(c);
        
        adjacency_list* ListAddition = PushStruct(Scratch, adjacency_list);
        ListAddition->NodeHandle = Connection.DownstreamNodeHandle;
        
        s32 ContiguousNodeIndex = SparseToContiguousNodeMap[Connection.UpstreamNodeHandle.Index];
        ListAddition->Next = NeighborsLists[ContiguousNodeIndex];
        NeighborsLists[ContiguousNodeIndex] = ListAddition;
        
        // Increment the number of neighbors - stored in Radix
        NeighborsListLengths[ContiguousNodeIndex].Radix++;
    }
    
    // Sort by number of neighbors
    RadixSortInPlace(NeighborsListLengths, Workspace->Nodes.Used);
    
    char* OutputCharArray = PushArray(Scratch, char, 1024);
    string OutputString = MakeString(OutputCharArray, 0, 1024);
    
    PrintF(&OutputString, "Neighbors Lists: \n");
    for (u32 d = 0; d < Workspace->Nodes.Used; d++)
    {
        PrintF(&OutputString, "    %d: Node [ %d ] : neighbors { ", d, NeighborsListLengths[d].ID);
        
        adjacency_list* Neighbors = NeighborsLists[d];
        while (Neighbors)
        {
            PrintF(&OutputString, "%d, ", Neighbors->NodeHandle.Index);
            Neighbors = Neighbors->Next;
        }
        PrintF(&OutputString, " }\n");
    }
    NullTerminate(&OutputString);
    
    // This is a contiguous array. 
    b8* NodesVisited = PushArray(Scratch, b8, NodeCount);
    GSZeroArray(NodesVisited, b8, NodeCount);
    
    Workspace->SortedNodeHandles = PushArray(&Workspace->Storage, gs_list_handle, NodeCount);
    u32 SortedSparseNodeIndeciesUsed = 0;
    
    for (u32 n = 0; n < Workspace->Nodes.Used; n++)
    {
        gs_radix_entry SortedNeighborsCount = NeighborsListLengths[n];
        u32 NeighborCount = SortedNeighborsCount.Radix;
        u32 NodeIndex = SortedNeighborsCount.ID;
        gs_list_handle NodeHandle = Workspace->Nodes.GetEntryAtIndex(NodeIndex)->Handle;
        u32 ContiguousNodeIndex = SparseToContiguousNodeMap[NodeIndex];
        
        SortedSparseNodeIndeciesUsed = SortNodeNeighbors(ContiguousNodeIndex, NodeHandle, NeighborsLists, NodesVisited, Workspace->SortedNodeHandles, SortedSparseNodeIndeciesUsed, SparseToContiguousNodeMap);
    }
    
    Workspace->SparseToSortedNodeMap = SparseToContiguousNodeMap;
    for (u32 SortedIndex = 0; SortedIndex < NodeCount; SortedIndex++)
    {
        gs_list_handle SortedHandle = Workspace->SortedNodeHandles[SortedIndex];
        Workspace->SparseToSortedNodeMap[SortedHandle.Index] = SortedIndex;
    }
}

internal void
PushNodeOnWorkspace(s32 NodeSpecificationIndex, pattern_node_workspace* Workspace, memory_arena* Scratch)
{
    pattern_node* NewNode = Workspace->Nodes.TakeElement();
    NewNode->SpecificationIndex = NodeSpecificationIndex;
    
    UpdateSortedNodes(Workspace, Scratch);
}

internal void
PushNodeConnectionOnWorkspace(gs_list_handle UpstreamNodeHandle, u32 UpstreamPortIndex, gs_list_handle DownstreamNodeHandle, u32 DownstreamPortIndex, pattern_node_workspace* Workspace, memory_arena* Scratch)
{
    pattern_node_connection Connection = {};
    Connection.UpstreamNodeHandle = UpstreamNodeHandle; 
    Connection.DownstreamNodeHandle = DownstreamNodeHandle; 
    Connection.UpstreamPortIndex = UpstreamPortIndex; 
    Connection.DownstreamPortIndex = DownstreamPortIndex; 
    
    Workspace->Connections.PushElementOnBucket(Connection);
    UpdateSortedNodes(Workspace, Scratch);
}

#define FOLDHAUS_NODE_CPP
#endif // FOLDHAUS_NODE_CPP
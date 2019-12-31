internal void
PushNodeOnWorkspace(s32 NodeSpecificationIndex, pattern_node_workspace* Workspace)
{
    pattern_node* NewNode = Workspace->Nodes.TakeElement();
    NewNode->SpecificationIndex = NodeSpecificationIndex;
}

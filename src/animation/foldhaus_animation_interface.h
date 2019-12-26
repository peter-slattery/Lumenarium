
FOLDHAUS_INPUT_COMMAND_PROC(DeleteAnimationBlock)
{
    if (AnimationBlockHandleIsValid(State->SelectedAnimationBlockHandle))
    {
        RemoveAnimationBlock(State->SelectedAnimationBlockHandle, &State->AnimationSystem);
        State->SelectedAnimationBlockHandle = {0};
    }
}

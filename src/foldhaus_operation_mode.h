struct operation_mode
{
    input_command_registry Commands;
};

#define OPERATION_MODES_MAX 32
struct operation_mode_system
{
    s32 ActiveModesCount;
    operation_mode ActiveModes[OPERATION_MODES_MAX];
};

internal void
ActivateOperationMode (operation_mode Mode, operation_mode_system* System)
{
    Assert(System->ActiveModesCount < OPERATION_MODES_MAX);
    System->ActiveModes[System->ActiveModesCount++] = Mode;
}

internal void
DeactivateCurrentOperationMode (operation_mode_system* System)
{
    Assert(System->ActiveModesCount > 0);
    System->ActiveModesCount--;
}
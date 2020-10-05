//
// File: foldhaus_log.h
// Author: Peter Slattery
// Creation Date: 2020-02-05
//
#ifndef FOLDHAUS_LOG_H

enum log_entry_type
{
    LogEntry_Message,
    LogEntry_Error,
};

struct log_entry
{
    gs_string Message;
    log_entry_type Type;
};

#define LOG_ENTRIES_MAX 256
struct event_log
{
    log_entry Entries[LOG_ENTRIES_MAX];
    // Circular buffer head position
    u32 NextEntry;
};

#define LogMessage(_Log, _Message) PushLogEntry(_Log, MakeString(_Message), LogEntry_Message)
#define LogError(_Log, _Message) PushLogEntry(_Log, MakeString(_Message), LogEntry_Error)

internal void
PushLogEntry(event_log* Log, gs_string Message, log_entry_type Type)
{
    u32 NewLogIndex = Log->NextEntry++;
    if (Log->NextEntry >= LOG_ENTRIES_MAX)
    {
        Log->NextEntry = 0;
    }
    
    log_entry* NewEntry = Log->Entries + NewLogIndex;
    NewEntry->Message = Message;
    NewEntry->Type = Type;
}




#define FOLDHAUS_LOG_H
#endif // FOLDHAUS_LOG_H
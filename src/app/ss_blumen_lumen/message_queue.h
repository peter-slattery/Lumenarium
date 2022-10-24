/* date = March 27th 2021 3:07 pm */

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#define BLUMEN_MESSAGE_QUEUE_COUNT 32
typedef struct blumen_network_msg_queue
{
    gs_data Buffers[BLUMEN_MESSAGE_QUEUE_COUNT];
    u32 WriteHead;
    u32 ReadHead;
} blumen_network_msg_queue;

// KB(1) is just bigger than any packet we send. Good for now
#define DEFAULT_QUEUE_ENTRY_SIZE KB(1)

#endif //MESSAGE_QUEUE_H

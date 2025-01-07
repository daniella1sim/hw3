#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H
#include <linux/ioctl.h>

#define BUFFER_SIZE 128
#define MAJOR_NUMBER 235
#define MESSAGE_SLOT_CHANNEL _IOW(MAJOR_NUMBER, 0, unsigned int)
#define DEVICE_NAME "message slot"

struct channel_node{
    unsigned int channel_id;
    char message[BUFFER_SIZE];
    int message_len;
    struct channel *next;
};

struct message_slot{
    struct channel *channel_list
};

#endif

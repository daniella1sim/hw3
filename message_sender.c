#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

#include "message_slot.h"

int main(int argc, char *argv[]){
    if (argc != 4){
        perror("Error in usage: <filename> <device_path> <channel_id> <message>\n");
        exit(1);
    }

    char *message_slot_file_path = argv[1];

    /*open specified message slot device file*/
    int fd = open(message_slot_file_path , O_WRONLY);
    if (fd < 0){
        perror("Error: failed opening device file\n");
        exit(1);
    }

    /* set the channel ID to the ID specified on the command line*/
    unsigned int target_message_channel_id = atoi(argv[2]);
    char* message = argv[3];
    int ret;

    
    ret = ioctl(fd, MESSAGE_SLOT_CHANNEL, target_message_channel_id);
    if (ret < 0){
        close(fd);
        perror("Error: failed setting channel id\n");
        exit(1);
    }

    /*write a message from the message slot to a buffer*/
    ret = write(fd, message, strlen(message));
    if (ret != strlen(message)){
        close(fd);
        perror("Error: failed writing message\n");
        exit(1);
    }    
    
    close(fd);
    exit(0);
}
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include "message_slot.h"

MODULE_LICENSE("GPL");

static struct message_slot device_table[257];

static int device_open(struct inode *inode, struct file *file){
    struct message_slot *slot;

    slot = kmalloc(sizeof(struct message_slot), GFP_KERNEL);    
    if (!slot){
        printk(KERN_ERR "Error: failed to allocate memory for message\n");
        return -ENOMEM; 
    }

    slot->channel_list = NULL;
    file->private_data = slot;
    return 0;
}


static ssize_t device_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset){
    struct channel *chan;
    if (!buffer){
        printk(KERN_ERR "Error: buffer pointer is NULL");
        return -EINVAL;
    }

    chan = file->private_data;

    if (!chan){
        printk(KERN_ERR "Error: channel not set correctly\n");
        return -EINVAL;
    } 
    
    if(len == 0 || len > BUFFER_SIZE){
        printk(KERN_ERR "Error: message len should be between 1 and 128\n");
        return -EMSGSIZE;
    }

    if (copy_to_user(chan->message, buffer, len)){
        printk(KERN_ERR "Error: failed to copy message\n");
        return -EFAULT;
    }
    printk("wrote messge from channel %d and minor %d", chan->id, iminor(file->f_inode));
    return chan->message_len;
}


static ssize_t device_read(struct file *file, char __user *buffer, size_t len, loff_t *offset){
    struct channel *chan;
    if (!buffer){
        printk(KERN_ERR "Error: buffer pointer is NULL");
        return -EINVAL;
    }
    chan = file->private_data;

    if (!chan){
        printk(KERN_ERR "Error: channel not set correctly\n");
        return -EINVAL;
    } 
    
    if(chan->message_len == 0){
        printk(KERN_ERR "Error: no message in channel\n");
        return -EWOULDBLOCK;
    }

    if (len < chan->message_len){
        printk(KERN_ERR "Error: buffer too small\n");
        return -ENOSPC;
    }

    if (copy_to_user(buffer, chan->message, chan->message_len)){
        printk(KERN_ERR "Error: failed to copy message\n");
        return -EFAULT;
    }
    printk("read messge from channel %d and minor %d", chan->id, iminor(file->f_inode));
    return chan->message_len;
}


static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param){
    struct message_slot *slot;
    struct channel *chan;
    unsigned int channel_id;
    slot = file->private_data;
    

    channel_id = (unsigned int)ioctl_param;

    if (ioctl_command_id != MESSAGE_SLOT_CHANNEL || channel_id == 0){
        printk("Error: invalid arguments. either channel ID is 0 or the ioctl command is not MSG_SLOT_CHANNEL\n");
        return -EINVAL;
    }
    
    chan = slot->channel_list;
    
    while (chan){
        if (chan->id == channel_id){
            break;
        }
        chan = chan->next;
    }

    if (!chan){
        chan = kmalloc(sizeof(struct channel), GFP_KERNEL);
        if (!chan){
            printk("Error: unable to allocate memory\n");
            return -ENOMEM;
        }
        chan->id = channel_id;
        chan->message_len = 0;
        chan->next = slot->channel_list;
        slot->channel_list = chan;
    }

    file->private_data = chan;
    return 0;
}


static void free_channels(struct channel *chan){
    struct channel *tmp;
    while (chan){
        tmp = chan;
        chan = chan->next;
        kfree(tmp);
    }
}

static void free_massage_slot(struct message_slot *slot){
    if (slot){
        free_channels(slot->channel_list);
        kfree(slot);
    }
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .write = device_write,
    .read = device_read,
};

static int __init message_slot_init(void){
    int i;
    int ret = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &fops);
    if (ret < 0){
        printk(KERN_ERR "device regirtration failed for %d\n", MAJOR_NUMBER);
        return ret;
    }

    for (i = 0; i < 257; i++){
        device_table[i].channel_list = NULL;
    }

    return 0;
}

static void __exit message_slot_exit(void){
    int i;
    for (i = 0; i < 256; i++){
        free_massage_slot(&device_table[i]);
    }
    unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
}

module_init(message_slot_init);
module_exit(message_slot_exit)

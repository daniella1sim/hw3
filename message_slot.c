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
    unsigned int minor = iminor(inode);
    struct message_slot *slot;

    if (minor >= 257){
        printk(KERN_ERR "Error: invalid minor %u\n", minor);
        return -ENODEV;
    }

    slot = &device_table[minor];
    file->private_data = slot;
    printk(KERN_INFO "device opened - minor %u\n",minor);
    return 0;
}


static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param){
    struct message_slot *slot;
    struct channel *chan;
    unsigned int channel_id = (unsigned int)ioctl_param;;

    
    if (ioctl_command_id != MSG_SLOT_CHANNEL || channel_id == 0){
        printk("Error: invalid arguments. either channel ID is 0 or the ioctl command is not MSG_SLOT_CHANNEL\n");
        return -EINVAL;
    }
    
    slot = file->private_data;
    
    chan = slot->channel_list;
    while (chan){
        if (chan->id == channel_id){
            file->private_data = chan;
            printk(KERN_INFO "device ioctl found existing channel ID %u\n",channel_id);
            return 0;
        }
        chan = chan->next;
    }

    chan = kmalloc(sizeof(struct channel), GFP_KERNEL);
    if (!chan){
        printk(KERN_ERR "Error: unable to allocate memory\n");
            return -ENOMEM;
        }
    chan->id = channel_id;
    chan->message_len = 0;
    chan->next = slot->channel_list;
    slot->channel_list = chan;
    
    file->private_data = chan;
    printk(KERN_INFO "device ioctl created new channel\n");
    return 0;
}


static ssize_t device_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset){
    struct channel *chan = file->private_data;
    
    if (!chan){
        printk(KERN_ERR "Error: channel not set correctly\n");
        return -EINVAL;
    } 
    
    if (!buffer){
        printk(KERN_ERR "Error: buffer pointer is NULL\n");
        return -EINVAL;
    }
    
    if(len == 0 || len > BUFFER_SIZE){
        printk(KERN_ERR "Error: message len should be between 1 and 128\n");
        return -EMSGSIZE;
    }

    if (copy_from_user(chan->message, buffer, len)){
        printk(KERN_ERR "Error: failed to copy message\n");
        return -EFAULT;
    }
    
    chan->message_len = len;
    printk(KERN_INFO "wrote messge from channel %u and minor %u of %ld bytes\n", chan->id, iminor(file->f_inode),len);
    return len;
}


static ssize_t device_read(struct file *file, char __user *buffer, size_t len, loff_t *offset){
    struct channel *chan = file->private_data;
    
    if (!chan){
        printk(KERN_ERR "Error: channel not set correctly\n");
        return -EINVAL;
    } 
    
    if (!buffer){
        printk(KERN_ERR "Error: buffer pointer is NULL");
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
    printk(KERN_INFO "read messge from channel %d and minor %d", chan->id, iminor(file->f_inode));
    return chan->message_len;
}


static void free_channels(struct channel *chan){
    struct channel *tmp;
    while (chan){
        tmp = chan;
        chan = chan->next;
        kfree(tmp);
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
    int ret;
    printk(KERN_INFO "messag slot init begin");
    ret = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &fops);
    if (ret < 0){
        printk(KERN_ERR "device registration failed for %d\n", MAJOR_NUMBER);
        return ret;
    }

    for (i = 0; i < 257; i++){
        device_table[i].channel_list = NULL;
        printk(KERN_INFO "message slot init:initialized slot %d\n",i);
            
    }

    printk(KERN_INFO "Message slot module loaded");
    return 0;
}

static void __exit message_slot_exit(void){
    int i;
    
    printk(KERN_INFO "message_slot_exit: start\n");
    
    for (i = 0; i < 257; i++){
        if (device_table[i].channel_list){
            printk(KERN_INFO "message slot exit: freeing channel for minior %d\n",i);

            free_channels(device_table[i].channel_list);
            device_table[i].channel_list = NULL;    
        }
    }
    unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
    printk(KERN_ERR "Message slot unloaded");
}

module_init(message_slot_init);
module_exit(message_slot_exit)

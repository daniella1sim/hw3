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

static int device_open(struct inode *inode, struct file *file){
    unsigned int minor = iminor(inode);//check this line
    struct message_slot *slot;

    slot = kmalloc(sizeof(struct message_slot), GFP_KERNEL);    
    if (!slot){
        printk(KERN_ERROR, "Error: failed to allocate memory for message\n")
        return -ENOMEM; 
    }

    slot->channel_list = NULL;
    file->private_data = slot;
    return 0;
}


static ssize_t device_write(struct file *file, char __user *buffer, size_t len, loff_t *offset){
    if (!buffer){
        printk(KERN_ERR, "Error: buffer pointer is NULL");
        return -EINVAL;
    }

    struct channel *chan = file->private_data;

    if (!chan){
        printk(KERN_ERROR, "Error: channel not set correctly\n")
        return -EINVAL;
    } 
    
    if(len == 0 or len > BUFFER_SIZE){
        printk(KERN_ERROR, "Error: message len should be between 1 and 128\n")
        return -EMSGSIZE;
    }

    if (copy_to_user(chan->message, buffer, len)){
        printk(KERN_ERROR, "Error: failed to copy message\n")
        return -EFAULT;
    }
    printk("wrote messge from channel %d and minor %d", chan, iminor(file->f_node))
    return chan->message_len;
}


static ssize_t device_read(struct file *file, char __user *buffer, size_t len, loff_t *offset){
    if (!buffer){
        printk(KERN_ERR, "Error: buffer pointer is NULL");
        return -EINVAL;
    }
    struct channel *chan = file->private_data;

    if (!chan){
        printk(KERN_ERROR, "Error: channel not set correctly\n")
        return -EINVAL;
    } 
    
    if(chan->message_len == 0){
        printk(KERN_ERROR, "Error: no message in channel\n")
        return -EWOULDBLOCK;
    }

    if (len < chan->message_len){
        printk(KERN_ERROR, "Error: buffer too small\n")
        return -ENOSPC;
    }

    if (copy_to_user(buffer, chan->message, chan->message_len)){
        printk(KERN_ERROR, "Error: failed to copy message\n")
        return -EFAULT;
    }
    printk("read messge from channel %d and minor %d", chan, iminor(file->f_node))
    return chan->message_len;
}


static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param){
    struct message_slot *slot = file->private_data;

    unsigned int channel_id = (unsigned int)ioctl_param;

    if (ioctl_command_id != MESSAGE_SLOT_CHANNEL || channel_id == 0){
        printk("Error: invalid argument")
        return EINVAL;
    }
    
    struct channel *chan = slot->channels;
    while (chan){
        if (chan->id == channel_id){
            break;
        }
        chan = chan->next;
    }

    if (!chan){
        chan = kmalloc(sizeof(struct channel), GFP_KERNEL);
        if (!chan){
            printk("Error: could not allocate memory");
            return -ENOMEM
        }
        chan->id = channel_id;
        chan->message_len = 0;
        chan->next = slot->channels;
        slot->channels = chan
    }

    file->private_data = chan;
    return 0;
}


static void free_channels(struct channel *chan){
    struct channel *temp;
    while (chan){
        tmp = chan;
        chan = chan->next;
        kfree(tmp);
    }
}

static void free_massage_slot(struct message_slot *slot){
    if (slot){
        free_channels(slot->channels);
        kfree(slot);
    }
}

static struct file_operation fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .write = device_write,
    .read = device_read,
};

static void __init message_slot_init(void){
    int ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops)
    if (ret < 0){
        printk(KERN_ERR, "Error: %s could not init for %d\n", DEVICE_NAME, MAJOR_NUMBER)
        return ret;
    }
    return 0;
}

static void __exit message_slot_exit(void){
    int i;
    for (i = 0; i < 256; i++){
        free_massage_slot(device_table[i]);
    }
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

module_init(message_slot_init);
module_exit(message_slot_exit)

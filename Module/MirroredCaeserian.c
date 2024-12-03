/**************************************************************
* Class::  CSC-415-01 Fall 2024
* Name:: Ulices Gonzalez
* Student ID:: 923328897
* GitHub-Name:: ulicessgg
* Project:: Assignment 6 – Device Driver
*
* File:: <MirroredCaeserian.c>
*
* Description:: Device Driver that mirrors a string and uses
* a Caeserian Cipher to Encrypt/Decrypt the string
*
**************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>

#include <linux/sched.h>

#define MY_MAJOR    415
#define MY_MINOR    0
#define DEVICE_NAME "MirroredCaeserian"

int major, minor;
char* kernel_buffer;

struct cdev my_cdev;
int actual_rx_size = 0;

MODULE_AUTHOR("Ulices Gonzalez");
MODULE_DESCRIPTION("A simple encryption program");
MODULE_LICENSE("GPL");

// data structure used for storing info essesntial to
typdef struct myCipher
{
    int numChars;
    int* key;   // array used for shifting characters can be uniformly set or randomized
    char* cipher;   // used to store text after encryption
    char* plainText;    // used to store text after decrption
} cipher;

static ssize_t myWrite(struct file* fs, const char __user* buf, size_t hsize, loff_t* off)
{
    struct myCipher* c = (struct myCipher*) fs->private_data;
    
    // TODO: need to store the amount of characters input by the user along with storing
    // the users original string

    // possible implementation -- still may need adjustment
    // c->numChars = strlen(buf);   // set number of characters
    // c->plainText = malloc(c->numChars);  // allocate the memory before any process
    // memcpy(c->plainText, buf, c->numChars)   // save text before terminating successfully

    // printk(KERN_INFO "We wrote: %lu characters", hsize);

    return hsize;
}

static int myOpen(struct inode* inode, struct file* fs)
{
    struct myCipher* c = vmalloc(sizeof(struct myCipher));

    if(c == 0)
    {
        printk(KERN_ERR "Can't vmalloc, File not opened.\n");
        return -1;        
    }

    //TODO: need to set values according to myCipher and after figuring out fs

    // possible implementation -- still may need adjustment
    // c->numChars = 0;
    // fs->private_data = c;

    return 0;
}

static int myClose(struct inode* inode, struct file* fs)    // think its done?
{
    struct myCipher* c = (struct myCipher*) fs->private_data;
    vfree(c);
    return 0;
}

static long myIoCtl(struct file* fs, unsigned int command, unsigned long data)
{
    int* count;
    struct myCipher* c = (struct myCipher*) fs->private_data;

    if(command != 3)
    {
        printk(KERN_ERR "Failed in myIoCtl.\n"); // virtual form of printf i believe must confirm
        return -1
    }

    count = (int*) data;
    int bytesNotCopied = copy_to_user(count, &(c->numChars), sizeof(int));
    // *count = c->numChars;

    return bytesNotCopied;
}

// data struct for using driver -- may need to be edited as cipher is fully implemented
struct file_operations fops = 
{
    .open = myOpen,
    .release = myClose,
    .write = myWrite,
    .unlocked_ioctl = myIoCtl,
    .owner = THIS_MODULE,
};

// creates device node in /dev, returns error if not made -- need to edit accordingly!
int init_module(void)
{
    int result, registers;
    dev_t devno = MKDEV/(MY_MAJOR, MY_MINOR);

    registers = register_chrdev_region(devno, 1, DEVICE_NAME);
    printk(KERN_INFO "Register chardev succeeded 1: %d\n", registers);
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    result = cdev_add(&my_cdev, devno, 1);
    printk(KERN_INFO "Dev Add chardev succeeed 2: %d\n", result);
    printk(KERN_INFO "Welcome - Mirrored Caeserian Driver is loaded.\n");

    if(result < 0)
    {
        printk(KERN_INFO "Register chardev failed: %d\n", result);
    }

    return result;
}

// unregistering and removing device from kernel -- not sure if i need to edit this
void cleanup_module(void)
{
    dev_t devno = MKDEV/(MY_MAJOR, MY_MINOR);
    unregister_chrdev_region(devno,1);
    cdev_del(&my_cdev);

    printk(KERN_INFO "Goodbyte from the Mirrored Caeserian Driver!\n");
}
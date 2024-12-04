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

#define MY_MAJOR       415
#define MY_MINOR       0
#define DEVICE_NAME   "MirroredCaeserian"

#define SETUNIKEYS     0
#define GENRANDKEYS    1

int major, minor;
char* kernel_buffer;

struct cdev my_cdev;
int actual_rx_size = 0;

MODULE_AUTHOR("Ulices Gonzalez");
MODULE_DESCRIPTION("A simple encryption program");
MODULE_LICENSE("GPL");

// data structure used for storing info essesntial to
struct myCipher
{
    int numChars;   // stores the total amount of characters input by the user
    char* buffer;   // stores text will be overwritten if already written upon encryption or decryption
    int* key;   // array used for shifting characters can be uniformly set or randomized
} cipher;

// writes text from user and allocates memory to support n amount of characters
static ssize_t myWrite(struct file* fs, const char __user* buf, size_t hsize, loff_t* off)
{
    struct myCipher* c = (struct myCipher*) fs->private_data;

    c->numChars = hsize;   // set number of characters

    // allocates memory to store text
    c->buffer = vmalloc(c->numChars);
    if(c->buffer == NULL)
    {
        printk(KERN_ERR "Can't vmalloc the buffer.\n");
        return -1;        
    }

    // allocates memory to store keys for each character
    c->key = vmalloc(c->numChars);
    if(c->key == NULL)
    {
        printk(KERN_ERR "Can't vmalloc keys.\n");
        return -1;        
    }

    // save text before terminating successfully
    if(copy_from_user(c->string, buf, c->numChars)) // Report error and exit forcefully if copy failed
    {
        printk(KERN_ERR "Failed to write.\n");
        return -1;
    }

    printk(KERN_INFO "We wrote: %lu characters", hsize);

    return hsize;
}

static ssize_t myRead(struct file* fs, char __user* buf, size_t hsize, loff_t* off)
{
    // TODO: This is where i will prompt encrypt and decrypt using an interface
    // should use switch statement and add some way to prompt users to set values

    // will include this in switch cases so parameters can be changed as needed
    if(copy_to_user(buf, c->string, c->numChars))  // save text before terminating successfully
    {
        printk(KERN_ERR "Failed to read.\n");  // Report error and exit forcefully if copy failed
        return -1;
    }
    
    return hsize;
}

// initalizes cipher instance for reading and or writing
static int myOpen(struct inode* inode, struct file* fs)
{
    // creates cipher struct instance and allocates memory for use
    struct myCipher* c = vmalloc(sizeof(struct myCipher));
    if(c == NULL)
    {
        printk(KERN_ERR "Can't vmalloc, File not opened.\n");
        return -1;        
    }

    // sets the size of current buffer(s) to be empty
    c->numChars = 0;

    // saves our struct instance for future use
    fs->private_data = c;

    return 0;
}

static int myClose(struct inode* inode, struct file* fs)
{
    struct myCipher* c = (struct myCipher*) fs->private_data;

    // free allocated memory
    vfree(c->buffer);
    vfree(c->key);
    vfree(c);

    // clear values after freeing allocated memory
    c->numChars = 0;
    c->buffer = NULL;
    c->key = NULL;
    c = NULL;

    return 0;
}

static long myIoCtl(struct file* fs, unsigned int command, unsigned long data)
{
    int* count;
    struct myCipher* c = (struct myCipher*) fs->private_data;

    if(command != 3)
    {
        printk(KERN_ERR "Failed in myIoCtl.\n"); // virtual form of printf i believe must confirm
        return -1;
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
    .read = myRead,
    .unlocked_ioctl = myIoCtl,
    .owner = THIS_MODULE,
};

// creates device node in /dev, returns error if not made -- need to edit accordingly!
int init_module(void)
{
    int result, registers;
    dev_t devno = MKDEV(MY_MAJOR, MY_MINOR);

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
    dev_t devno = MKDEV(MY_MAJOR, MY_MINOR);
    unregister_chrdev_region(devno, 1);
    cdev_del(&my_cdev);
    printk(KERN_INFO "Goodbyte from the Mirrored Caeserian Driver!\n");
}
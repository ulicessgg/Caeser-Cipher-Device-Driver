/**************************************************************
* Class::  CSC-415-01 Fall 2024
* Name:: Ulices Gonzalez
* Student ID:: 923328897
* GitHub-Name:: ulicessgg
* Project:: Assignment 6 – Device Driver
*
* File:: <ReversedAlternatingCaeserian.c>
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

// used for random key generation
#include <linux/random.h>

#define MY_MAJOR       415
#define MY_MINOR       0
#define DEVICE_NAME    "ReversedAlternatingCaeserian"

int major, minor;
char* kernel_buffer;

struct cdev my_cdev;
int actual_rx_size = 0;

MODULE_AUTHOR("Ulices Gonzalez");
MODULE_DESCRIPTION("A simple encryption program");
MODULE_LICENSE("GPL");

// function used to reverse strings for ALL options
int reverse(char**, int);

// general encryption and decryption functions used for driver 
int encrypt(char**, int, int);
int decrypt(char**, int, int);

// functions used when one time pad is selected
int setPad(int**, int);
int otpEncrypt(char**, int);

// data structure used for storing info essesntial to
struct myCipher
{
    int numChars;   // stores the total amount of characters input by the user
    char* buffer;   // stores text for encryption and decryption will be overwritten
    int* key;    // used for shifting characters for both encryption and decryption
} myCipher;

int mode;   // used to select between encryption or decryption

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

    // save text before terminating successfully
    if(copy_from_user(c->buffer, buf, c->numChars)) // Report error and exit forcefully if copy failed
    {
        printk(KERN_ERR "Failed to write.\n");
        return -1;
    }

    c->buffer[c->numChars] = '\0';  // sets the null terminator for future use

    printk(KERN_INFO "We wrote: %lu characters", hsize);

    return hsize;
}

static ssize_t myRead(struct file* fs, char __user* buf, size_t hsize, loff_t* off)
{
    struct myCipher* c = (struct myCipher*) fs->private_data;

    
    // TODO: This is where i will prompt encrypt and decrypt using an interface
    // should use switch statement and add some way to prompt users to set values

    if(copy_to_user(buf, c->buffer, c->numChars))  // save text before terminating successfully
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

    // sets the size of current buffer to 0 and allocates memory for key
    c->numChars = 0;
    c->key = vmalloc(sizeof(int));
    if(c->key == NULL)
    {
        printk(KERN_ERR "Can't vmalloc key.\n");
        return -1;        
    }

    // saves our struct instance for future use
    fs->private_data = c;

    return 0;
}

static int myClose(struct inode* inode, struct file* fs)
{
    struct myCipher* c = (struct myCipher*) fs->private_data;

    // free allocated memory
    vfree(c->buffer);
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
    struct myCipher* c = (struct myCipher*) fs->private_data;

    // save text before terminating successfully
    if(copy_from_user(c->key, (int __user*) data, sizeof(int)))
    {
        // Report error and exit forcefully if copy failed
        printk(KERN_ERR "Failed to copy key.\n");
        return -1;
    }

    // selects process based on command entered by user
    switch(command)
    {
        case 0: // sets mode for encrypting user text
            mode = 0;
            break;
        case 1: // sets mode for decrypting user text
            mode = 1;
            break;
        case 2: // sets mode for encrypting with one time pad, decryption impossible if attempted
            mode = 2;
            break;
        default:    // forcefully returns if command is invalid
            printk(KERN_ERR "Failed in myIoCtl.\n");
            return -1;
    }

    fs->private_data = c;

    return 0;
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

// creates device node in /dev, returns error if not made
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

// unregistering and removing device from kernel
void cleanup_module(void)
{
    dev_t devno = MKDEV(MY_MAJOR, MY_MINOR);
    unregister_chrdev_region(devno, 1);
    cdev_del(&my_cdev);
    printk(KERN_INFO "Goodbyte from the Mirrored Caeserian Driver!\n");
}

//////////////////////////////////////////////////////////////////////////////

// im a fan of da vinci and he was known for mirror writing in reverse
// so in an effort to add some complexity i will be doing the same here
// reversed text will be returned through reference
int reverse(char** buffer, int numChars)
{
    // create temp buffer and allocates memory to hold reversed values, prevents buffering issues
    char* tempBuffer = vmalloc(numChars);
    if(tempBuffer == NULL) 
    {
        // Report error and exit forcefully if copy failed
        printk(KERN_ERR "Can't vmalloc the temporary buffer in Reverse.\n");
        return -1;        
    }
    
    // saves characters into our temp buffer as we iterate backwards through
    // the source buffer
    for(int i = 0; i < numChars; i++)
    {
        for(int j = numChars - 1; j >= 0; j--)
        {
            tempBuffer[i] = *buffer[j];
        }
    }

    // sets the null terminator and copies back to our buffer once done
    tempBuffer[numChars] = '\0';
    *buffer = tempBuffer;

    // free the allocated memory before terminating
    vfree(tempBuffer);

    return 0;
}

// encrypts supplied buffer with provided key and returns cipher by reference
int encrypt(char** buffer, int numChars, int key)
{
    // shifts characters using key and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        if(i % 2 == 0)  // if even shifts up
        {
            buffer[i] = buffer[i] + key;
        }
        else    // if odd shifts down
        {
            buffer[i] = buffer[i] - key;
        }
    }

    // signifies successful encryption
    return 0;
}

// decrypts supplied buffer with provided key and returns plain text by reference
int decrypt(char** buffer, int numChars, int key)
{
    // shifts characters using key and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        if(i % 2 == 0)  // if even shifts down
        {
            buffer[i] = buffer[i] - key;
        }
        else    // if odd shifts up
        {
            buffer[i] = buffer[i] + key;
        }
    }

    // signifies successful decryption
    return 0;
}

// sets one time pad using random keys for encryption and decryption
// returns pad by reference
int setPad(int** pad, int numChars)
{
    // create and allocate temporary pad for generation
    int* tempPad = vmalloc(numChars);

    // generates random 0 to numChars and saves to the temp pad
    for(int i = 0; i < numChars; i++)
    {
        int temp;   // holds random value
        get_random_bytes(&temp, numChars); // need to test before use but generates random keys

        tempPad[i] = temp % numChars; // reduces the random key and saves it to the pad
    }

    // signifies successful encryption
    return 0;
}

// encrypts supplied buffer with random keys and returns cipher by reference
int otpEncrypt(char** buffer, int numChars)
{
    // create and set the one time pad
    int* pad = vmalloc(numChars);
    setPad(&pad, numChars);

    // shifts characters using ne time pad and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        if(i % 2 == 0)  // if even shifts up
        {
            buffer[i] = buffer[i] + pad[i];
        }
        else    // if odd shifts down
        {
            buffer[i] = buffer[i] - pad[i];
        }
    }

    // signifies successful encryption
    return 0;
}
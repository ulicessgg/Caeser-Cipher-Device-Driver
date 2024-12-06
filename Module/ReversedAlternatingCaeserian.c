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
#define BUFFER_SIZE    244

int major, minor;
char* kernel_buffer;

struct cdev my_cdev;
int actual_rx_size = 0;

MODULE_AUTHOR("Ulices Gonzalez");
MODULE_DESCRIPTION("A simple encryption program");
MODULE_LICENSE("GPL");

// function used to reverse strings for ALL options
char* reverse(char*, int);

// general encryption and decryption functions used for driver 
char* encrypt(char*, int, int);
char* decrypt(char*, int, int);

// functions used when one time pad is selected
int* setPad(int*, int);
char* otpEncrypt(char*, int);

// data structure used for storing info essesntial to
struct myCipher
{
    int numChars;   // stores the total amount of characters input by the user
    char* buffer;   // stores text for encryption and decryption will be overwritten
    int key;    // used for shifting characters for both encryption and decryption
    int mode;   // used to select between encryption or decryption
} myCipher;

// writes text from user and allocates memory to support n amount of characters
static ssize_t myWrite(struct file* fs, const char __user* buf, size_t hsize, loff_t* off)
{
    struct myCipher* c = (struct myCipher*) fs->private_data;

    c->numChars = hsize;   // set number of characters

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

    hsize = c->numChars;

    // executes user processes based off mode set in ioctl
    switch(c->mode)
    {
        case 0: // encrypts the users string with their key
            c->buffer = encrypt(c->buffer, c->numChars, c->key);
            break;
        case 1: // decrypts the users string with their key
            c->buffer = decrypt(c->buffer, c->numChars, c->key);
            break;
        case 2: // generates one time pad and encrypts users string
            c->buffer = otpEncrypt(c->buffer, c->numChars);
            break;
        default:
            printk(KERN_ERR "Failed in myRead.\n");
            return -1;
    }

    if(copy_to_user(buf, c->buffer, hsize))  // save text before terminating successfully
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

    c->buffer = vmalloc(BUFFER_SIZE);
    if(c->buffer == NULL)
    {
        printk(KERN_ERR "Can't vmalloc buffer.\n");
        return -1;        
    }

    c->key = 0;

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
    c->key = 0;
    c = NULL;

    return 0;
}

static long myIoCtl(struct file* fs, unsigned int command, unsigned long data)
{
    struct myCipher* c = (struct myCipher*) fs->private_data;

    int* tempKey = 0;

    // save text before terminating successfully
    if(copy_from_user(tempKey, (int __user*) data, sizeof(c->key)))
    {
        // Report error and exit forcefully if copy failed
        printk(KERN_ERR "Failed to copy key.\n");
        return -1;
    }

    c->key = *tempKey;

    // selects process based on command entered by user
    switch(command)
    {
        case 0: // sets mode for encrypting user text
            c->mode = 0;
            break;
        case 1: // sets mode for decrypting user text
            c->mode = 1;
            break;
        case 2: // sets mode for encrypting with one time pad, decryption impossible if attempted
            c->mode = 2;
            break;
        default:    // forcefully returns if command is invalid
            printk(KERN_ERR "Failed in myIoCtl.\n");
            return -1;
    }

    fs-> private_data = c;

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
char* reverse(char* buffer, int numChars)
{
    // create temp buffer and allocates memory to hold reversed values, prevents buffering issues
    char* tempBuffer = vmalloc(numChars);
    
    // saves characters into our temp buffer as we iterate backwards through
    // the source buffer
    for(int i = 0; i < numChars; i++)
    {
        for(int j = numChars - 1; j >= 0; j--)
        {
            tempBuffer[i] = buffer[j];
        }
    }

    // sets the null terminator and copies back to our buffer once done
    tempBuffer[numChars] = '\0';
    buffer = tempBuffer;

    // free the allocated memory before terminating
    vfree(tempBuffer);

    return buffer;
}

// encrypts supplied buffer with provided key and returns cipher by reference
char* encrypt(char* buffer, int numChars, int key)
{
    char* tempBuffer = vmalloc(numChars);

    // shifts characters using key and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        if(i % 2 == 0)  // if even shift up
        {
            tempBuffer[i] = buffer[i] + key;
        }
        else    // if odd shifts down
        {
            tempBuffer[i] = buffer[i] - key;
        }
    }

    tempBuffer[numChars] = '\0';
    buffer = tempBuffer;
    vfree(tempBuffer);

    // signifies successful encryption
    return buffer;
}

// decrypts supplied buffer with provided key and returns plain text by reference
char* decrypt(char* buffer, int numChars, int key)
{
    char* tempBuffer = vmalloc(numChars);

    // shifts characters using key and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        if(i % 2 == 0)  // if even shift down
        {
            tempBuffer[i] = buffer[i] - key;
        }
        else    // if odd shifts up
        {
            tempBuffer[i] = buffer[i] + key;
        }
    }

    tempBuffer[numChars] = '\0';
    buffer = tempBuffer;
    vfree(tempBuffer);

    // signifies successful encryption
    return buffer;
}

// sets one time pad using random keys for encryption and decryption
// returns pad by reference
int* setPad(int* pad, int numChars)
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

    pad = tempPad;
    vfree(tempPad);

    // signifies successful encryption
    return pad;
}

// encrypts supplied buffer with random keys and returns cipher by reference
char* otpEncrypt(char* buffer, int numChars)
{
    // create and set the one time pad
    int* pad = vmalloc(numChars);
    pad = setPad(pad, numChars);

    char* tempBuffer = vmalloc(numChars);

    // shifts characters using ne time pad and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        if(i % 2 == 0)  // if even shifts up
        {
            tempBuffer[i] = buffer[i] + pad[i];
        }
        else    // if odd shifts down
        {
            tempBuffer[i] = buffer[i] - pad[i];
        }
    }

    tempBuffer[numChars] = '\0';
    buffer = tempBuffer;

    // signifies successful encryption
    return buffer;
}
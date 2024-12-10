/**************************************************************
* Class::  CSC-415-01 Fall 2024
* Name:: Ulices Gonzalez
* Student ID:: 923328897
* GitHub-Name:: ulicessgg
* Project:: Assignment 6 – Device Driver
*
* File:: <AlternatingCaeserian.c>
*
* Description:: Device Driver that uses an Alternating Caeser 
* Cipher to Encrypt/Decrypt user string with given key
* Can also generate one time pad and encrypt it in reverse
*
**************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/random.h>

#define MY_MAJOR       415
#define MY_MINOR       0
#define DEVICE_NAME    "AlternatingCaeserian"
#define BUFFER_SIZE    244

int major, minor;
char* kernel_buffer;

struct cdev my_cdev;
int actual_rx_size = 0;

MODULE_AUTHOR("Ulices Gonzalez");
MODULE_DESCRIPTION("A simple encryption program");
MODULE_LICENSE("GPL");

//  general encryption and decryption functions used for driver 
char* encrypt(char*, int, int);
char* decrypt(char*, int, int);

//  functions used when one time pad is selected
char* reverse(char*, int);
int randKey(int);
char* otpEncrypt(char*, int);

//  data structure used for storing info essesntial to
struct myCipher
{
    int numChars;   //  stores the total amount of characters input by the user
    int key;    //  used for shifting characters for both encryption and decryption
    int mode;   //  used to select between encryption or decryption
    char* buffer;   //  stores text for encryption and decryption will be overwritten
} myCipher;

//  writes text from user
static ssize_t myWrite(struct file* fs, const char __user* buf, size_t hsize, loff_t* off)
{
    //  load struct before writing
    struct myCipher* c = (struct myCipher*) fs->private_data;
    if(c == NULL)   //  report error and exit if loading failed
    {
        printk(KERN_ERR "Failed to load private data!");
        return -1;
    }

    c->numChars = hsize;   //   set number of characters

    //  save text before terminating successfully, report error and exit if copy failed
    if(copy_from_user(c->buffer, buf, c->numChars)) 
    {
        printk(KERN_ERR "Failed to write.\n");
        return -1;
    }

    c->buffer[c->numChars] = '\0';  //    sets the null terminator for future use

    printk(KERN_INFO "We wrote: %lu characters", hsize);

    return hsize;
}

//  reads text to user
static ssize_t myRead(struct file* fs, char __user* buf, size_t hsize, loff_t* off)
{
    //  load struct before reading
    struct myCipher* c = (struct myCipher*) fs->private_data;
    if(c == NULL)   //  report error and exit if loading failed
    {
        printk(KERN_ERR "Failed to load private data!");
        return -1;
    }

    //  using mode set by user command call respective function before copying to user
    switch(c->mode)
    {
        case 0:
            c->buffer = encrypt(c->buffer, c->numChars, c->key);
            if(c->buffer == NULL)   // report error and exit
            {
                printk(KERN_ERR "Failed to encrypt buffer");
                return -1;
            }
            break;
        case 1:
            c->buffer = decrypt(c->buffer, c->numChars, c->key);
            if(c->buffer == NULL)   // report error and exit
            {
                printk(KERN_ERR "Failed to encrypt buffer");
                return -1;
            }
            break;
        case 2:
            c->buffer = otpEncrypt(c->buffer, c->numChars);
            if(c->buffer == NULL)   // report error and exit
            {
                printk(KERN_ERR "Failed to encrypt buffer");
                return -1;
            }
            break;
        default:
            printk(KERN_ERR "Failed to execute user command.\n");  
            return -1;
    }

    //  save text before terminating successfully, report error and exit if copy failed
    if(copy_to_user(buf, c->buffer, hsize)) 
    {
        printk(KERN_ERR "Failed to read.\n"); 
        return -1;
    }

    printk(KERN_INFO "We read: %lu characters", hsize);

    return hsize;
}

//  initalizes cipher instance for reading and or writing
static int myOpen(struct inode* inode, struct file* fs)
{
    //  create cipher struct instance and allocate memory for use
    struct myCipher* c = (struct myCipher*) vmalloc(sizeof(struct myCipher));
    if(c == NULL)   // check if instance is allocated and report and exit if not
    {
        printk(KERN_ERR "Can't vmalloc, File not opened.\n");
        return -1;        
    }

    //  set the size of current buffer, mode, and key to 0 and allocate buffer
    c->numChars = 0;
    c->mode = 0;
    c->key = 0;
    c->buffer = (char*) vmalloc(BUFFER_SIZE);
    if(c->buffer == NULL)   //  check if buffer is allocated, report and exit if not
    {
        printk(KERN_ERR "Can't vmalloc buffer.\n");
        return -1;        
    }

    //  saves our struct instance for future use
    fs->private_data = c;

    return 0;
}

//  releases struct instance and before closing driver
static int myClose(struct inode* inode, struct file* fs)
{
    //  load struct before closing
    struct myCipher* c = (struct myCipher*) fs->private_data;
    if(c == NULL)   //  report error and exit if loading failed
    {
        printk(KERN_ERR "Failed to load private data!");
        return -1;
    }

    //  free allocated memory
    vfree(c->buffer);
    vfree(c);

    //  clear values after freeing allocated memory
    c->numChars = 0;
    c->key = 0;
    c->mode = 0;
    c->buffer = NULL;
    c = NULL;

    return 0;
}

//  sets user requested process and cipher key
static long myIoCtl(struct file* fs, unsigned int command, unsigned long data)
{
    //  load struct before setting key and commmand
    struct myCipher* c = (struct myCipher*) fs->private_data;
    if(c == NULL)   //  report error and exit if loading failed
    {
        printk(KERN_ERR "Failed to load private data!");
        return -1;
    }

    //  save key before setting command, report error and exit if copy failed
    if(copy_from_user(&(c->key), (int __user*) data, sizeof(c->key)))
    {
        printk(KERN_ERR "Failed to copy key.\n");
        return -EFAULT;
    }

    //  selects process based on command entered by user
    switch(command)
    {
        case 'e': // sets mode for encrypting user text
            c->mode = 0;
            break;
        case 'd': // sets mode for decrypting user text
            c->mode = 1;
            break;
        case 'o': // sets mode for encrypting with one time pad, decryption impossible if attempted
            c->mode = 2;
            break;
        default:    // forcefully returns if command is invalid
            printk(KERN_ERR "Failed in myIoCtl.\n");
            return -1;
    }

    //  saves our struct instance
    fs-> private_data = c;

    return 0;
}

//  data struct for using driver 
struct file_operations fops = 
{
    .open = myOpen,
    .release = myClose,
    .write = myWrite,
    .read = myRead,
    .unlocked_ioctl = myIoCtl,
    .owner = THIS_MODULE,
};

//  creates device node in /dev, returns error if not made
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
    printk(KERN_INFO "Welcome - The Alternating Caeserian Driver is loaded.\n");

    if(result < 0)
    {
        printk(KERN_INFO "Register chardev failed: %d\n", result);
    }

    return result;
}

//  unregistering and removing device from kernel
void cleanup_module(void)
{
    dev_t devno = MKDEV(MY_MAJOR, MY_MINOR);
    unregister_chrdev_region(devno, 1);
    cdev_del(&my_cdev);
    printk(KERN_INFO "Goodbyte from The Alternating Caeserian Driver!\n");
}

//  encrypts supplied buffer with provided key and returns cipher
char* encrypt(char* buffer, int numChars, int key)
{
    //  create temp buffer and allocate it
    char* tempBuffer = vmalloc(numChars);
    if(tempBuffer == NULL)  //  report and exit if allocation failed
    {
        printk(KERN_ERR "Failed to allocate temp buffer in encrypt");
        return NULL;
    }

    //  shifts characters using key and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        if(buffer[i] == ' ')    //  if space set it and skip
        {
            tempBuffer[i] = ' ';
        }
        else if(i % 2 == 0)  // if even shift up
        {
            tempBuffer[i] = buffer[i] + key;
        }
        else if(i % 2 != 0)    //   if odd shifts down
        {
            tempBuffer[i] = buffer[i] - key;
        }
    }

    //  sets the null terminator and copies back to our buffer once done
    tempBuffer[numChars] = '\0';
    strncpy(buffer, tempBuffer, numChars);
    
    //  free the allocated memory before terminating
    vfree(tempBuffer);

    //  signifies successful encryption
    return buffer;
}

// decrypts supplied buffer with provided key and returns plain text
char* decrypt(char* buffer, int numChars, int key)
{
    //  create temp buffer and allocate it
    char* tempBuffer = vmalloc(numChars);
    if(tempBuffer == NULL)  //  report and exit if allocation failed
    {
        printk(KERN_ERR "Failed to allocate temp buffer in decrypt");
        return NULL;
    }

    //  shifts characters using key and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        if(buffer[i] == ' ')    //  if space set it and skip
        {
            tempBuffer[i] = ' ';
        }
        else if(i % 2 == 0)  // if even shift back down
        {
            tempBuffer[i] = buffer[i] - key;
        }
        else if(i % 2 != 0)    //   if odd shifts back up
        {
            tempBuffer[i] = buffer[i] + key;
        }
    }

    //  sets the null terminator and copies back to our buffer once done
    tempBuffer[numChars] = '\0';
    strncpy(buffer, tempBuffer, numChars);
    
    //  free the allocated memory before terminating
    vfree(tempBuffer);

    //  signifies successful decryption
    return buffer;
}

//  im a fan of da vinci and he was known for mirror writing
//  so in an effort to add some complexity i will be doing the 
//  same here by reversing text in one time pad caeser cipher
char* reverse(char* buffer, int numChars)
{
    //  create temp buffer and allocate it
    char* tempBuffer = vmalloc(numChars);
    if(tempBuffer == NULL)  //  report and exit if allocation failed
    {
        printk(KERN_ERR "Failed to allocate temp buffer in reverse");
        return NULL;
    }
    
    //  iterates backwards and save to tempBuffer to reverse text
    for(int i = 0; i < numChars; i++)
    {
        tempBuffer[i] = buffer[numChars - i - 1];
    }

    //  sets the null terminator and copies back to our buffer once done
    tempBuffer[numChars] = '\0';
    strncpy(buffer, tempBuffer, numChars);

    //  free the allocated memory before terminating
    vfree(tempBuffer);

    return buffer;
}

//  sets one time pad using random keys for encryption and decryption
int randKey(int numChars)
{
    //  create a tempKey for random values
    unsigned int tempKey = 0;

    //  generate random values and save to tempKey
    get_random_bytes(&tempKey, sizeof(unsigned int));
    tempKey = (tempKey % numChars) + 1; // reduce the random number and save it

    // signifies successful generation
    return tempKey;
}

//  encrypts supplied buffer with random keys and returns cipher
char* otpEncrypt(char* buffer, int numChars)
{
    //  create and set the one time pad
    int key = 0;

    //  create temp buffer and allocate it
    char* tempBuffer = vmalloc(numChars);
    if(tempBuffer == NULL)  //  report and exit if allocation failed
    {
        printk(KERN_ERR "Failed to allocate temp buffer in reverse");
        return NULL;
    }

    //  reverse user buffer before encrypting
    buffer = reverse(buffer, numChars);
    if(buffer == NULL)   //  report error and exit
    {
        printk(KERN_ERR "Failed to reverse buffer");
        return NULL;
    }

    //  shifts characters using one time pad and alternates shift each index
    for(int i = 0; i < numChars; i++)
    {
        key = randKey(numChars);
        if(buffer[i] == ' ')    //  if space set it and skip
        {
            tempBuffer[i] = ' ';
        }
        else if(i % 2 == 0)  // if even shift up
        {
            tempBuffer[i] = buffer[i] + key;
        }
        else if(i % 2 != 0)    //   if odd shifts down
        {
            tempBuffer[i] = buffer[i] - key;
        }
    }

    //  sets the null terminator and copies back to our buffer once done
    tempBuffer[numChars] = '\0';
    strncpy(buffer, tempBuffer, numChars);

    //  free the allocated memory before terminating
    vfree(tempBuffer);

    return buffer;
}
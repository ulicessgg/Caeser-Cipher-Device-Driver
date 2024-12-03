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
    size_t numChars;
    int* key;   // array used for shifting characters can be uniformly set or randomized
    char* cipher;   // used to store text after encryption
    char* plainText;    // used to store text after decrption
} cipher;
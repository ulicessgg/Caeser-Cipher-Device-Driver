/**************************************************************
* Class::  CSC-415-01 Fall 2024
* Name:: Ulices Gonzalez
* Student ID:: 923328897
* GitHub-Name:: ulicessgg
* Project:: Assignment 6 – Device Driver
*
* File:: <Gonzalez_Ulices_HW6_main.c>
*
* Description:: 
*
**************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#define PATH "/dev/ReversedAlternatingCaeserian"
#define ENCR "Encrypted"
#define DECR "Decrypted"
#define OTPS "One Time Pad Encrypted"
#define BUFFER_SIZE 244

int main(int argc, char* argv[])
{
    // open driver and create file descriptor for driver use
    int fd = open(PATH, O_RDWR);
    if(fd < 0)  // ensure file descriptor is valid to proceed
    {
        printf("Failed to open driver!");
        return -1;
    }

    // set string being encrypted/decrypted and gather its size
    char* string = argv[2];
    int numChars = strlen(string);

    if(numChars > BUFFER_SIZE)
    {
        numChars = BUFFER_SIZE;
    }

    printf("Original String: %s\n", string);

    // write string of numChars size to the driver module
    int retSize = write(fd, string, numChars);
    // if the returned size is not the same as what was passed then exit
    if(retSize != numChars)
    {
        printf("Failed to write to driver!");
        return -1;
    }

    printf("Wrote: %d bytes\n", retSize);

    // set the mode for the driver encrypt-e decrypt-d one_time_pad-o
    char command = argv[1][0];

    // set the encryption key for caeser cipher
    int key = atoi(argv[3]);

    char* out;

    switch(command)
    {
        case 'e': 
            out = ENCR;
            break;
        case 'd': 
            out = DECR;
            break;
        case 'o': 
            out = OTPS;
            break;
        default:
            printf("Invalid Mode.\n");
            return -1;
    }

    // ensure that ioctl is succesful before moving onto read
    if(ioctl(fd, command, &key) < 0)
    {
        printf("Failed to set command to driver!");
        return -1;
    }

    printf("Key Used: %d\n", key);

    // read from the driver and gather its return value
    retSize = read(fd, string, numChars);
    // if the returned size is not the same as what was passed then exit
    if(retSize != numChars)
    {
        printf("Failed to read from driver!");
        return -1;
    }

    printf("Reading: %d bytes\n", retSize);
    printf("%s String: %s\n", out, string);
    
    close(fd);

    return 0;
}
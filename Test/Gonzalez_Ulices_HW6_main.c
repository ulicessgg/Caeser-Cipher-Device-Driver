/**************************************************************
* Class::  CSC-415-01 Fall 2024
* Name:: Ulices Gonzalez
* Student ID:: 923328897
* GitHub-Name:: ulicessgg
* Project:: Assignment 6 – Device Driver
*
* File:: <Gonzalez_Ulices_HW6_main.c>
*
* Description:: Simple program that makes use of Alternating
* Caeserian Driver built in Kernel Module, allows users the
* ability to write string for encryption/decryption and set
* command and key for their intended process.
*
**************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define PATH "/dev/AlternatingCaeserian"
#define ENCR "Encrypted"
#define DECR "Decrypted"
#define OTPS "One Time Pad Encrypted"
#define BUFFER_SIZE 244

int main(int argc, char* argv[])
{
    //  create variables for driver use
    char* string = malloc(BUFFER_SIZE);
    char command;
    int key;

    if(argc == 4)   //  handles the use of RUNOPTIONS for inputting a string, command, and key
    {
        //  copy the users input directly into our variables to pass to the driver
        strncpy(string, argv[2], BUFFER_SIZE);
        command = argv[1][0];
        key = atoi(argv[3]);
    }
    else    //  handles the lack of RUNOPTIONS and promtps for user input string, command, and key
    {
        //  user is prompted for text and driver command
        printf("Enter Text for Encryption/Decryption: ");
        fgets(string, BUFFER_SIZE, stdin);

        printf("Enter e - for Encryption | d - for Decryption | o - for One Time Pad: ");
        scanf("%s", &command);

        //  used to avoid needless output to the terminal
        if(command != 'o')
        {
            //  user is prompted for key if one time pad is not selected
            printf("Enter Encryption/Decryption key: ");
            scanf("%d", &key);
        }
        else
        {
            key = 0;    // key is set to 0 as it wont be used for one time pad
        }
    }
    
    // open driver and create file descriptor for driver use
    int fd = open(PATH, O_RDWR);
    if(fd < 0)  // ensure file descriptor is valid to proceed
    {
        printf("Failed to open driver!");
        return -1;
    }

    // set string being encrypted/decrypted and limit its size before writing
    int numChars = strlen(string);
    if(numChars > BUFFER_SIZE)
    {
        numChars = BUFFER_SIZE;
    }

    printf("\nOriginal String: %s", string);

    // writes string to the driver module and checks for success beofre moving onto ioctl
    int retSize = write(fd, string, numChars);
    if(retSize != numChars)
    {
        printf("Failed to write to driver!");
        return -1;
    }

    printf("Wrote: %d bytes", retSize);

    char* outDesc;  //  outputs respective description of returned string from read
    switch(command) //  output description is to provide users accurate info
    {
        case 'e': 
            outDesc = ENCR;
            break;
        case 'd': 
            outDesc = DECR;
            break;
        case 'o': 
            outDesc = OTPS;
            break;
        default:
            printf("Invalid Mode.\n");
            return -1;
    }

    // call ioctl and check for success before moving onto read
    if(ioctl(fd, command, &key) < 0)
    {
        printf("Failed to set command to driver!");
        return -1;
    }

    // will output the users key if alternating caeser cipher is used
    if(command != 'o')
    {
        printf("\nKey Used: %d", key);
    }

    // read from the driver and check for success before terminating
    retSize = read(fd, string, numChars);
    if(retSize != numChars)
    {
        printf("Failed to read from driver!");
        return -1;
    }

    printf("\nReading: %d bytes", retSize);
    printf("\n%s String: %s\n", outDesc, string);
    
    close(fd);

    return 0;
}

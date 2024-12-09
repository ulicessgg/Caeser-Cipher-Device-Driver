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
#define BUFFER_SIZE 244

int main(int argc, char* argv[])
{
    int fd = open(PATH, O_RDWR);
    printf("error is in write: %d\n", fd);

    char* string = argv[2];

    write(fd, string, strlen(string));

    printf("error is in ioctl: %d\n", fd);

    char command = argv[1][0];

    char key = argv[3][0];

    ioctl(fd, command, key);

    printf("error is in read: %d\n", fd);


    char* buffer = malloc(strlen(string));

    read(fd, buffer, strlen(buffer));

    
    close(fd);
    printf("error is not in open: %d\n", fd);


    return 0;
}
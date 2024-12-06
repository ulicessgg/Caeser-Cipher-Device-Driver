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

#define PATH "/dev/ReverseAlternatingCaeserian"
#define BUFFER_SIZE 244

int main(int argc, char* argv[])
{
    int fd = open(PATH, O_RDWR);

    if(fd < 0)
    {
        printf("error is in open: %d\n", fd);
        return 1;
    }

    close(fd);

    return 0;
}
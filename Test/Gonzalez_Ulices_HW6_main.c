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

int main(int argc, char* argv[])
{
    char command = argv[1][0];
    char* userText = argv[2];
    int textLen = strlen(userText);
    char* keyChar = argv[3];
    int key = atoi(keyChar);
    
    char* buffer = malloc(textLen);

    printf("Original String: %s\n", userText);
    
    int fd = open("/dev/ReverseAlternatingCaeserian", O_RDWR);

    write(fd, userText, textLen);

    ioctl(fd, command, key);

    read(fd, buffer, textLen);

    printf("New String: %s\n", buffer);

    free(buffer);

    close(fd);

    return 0;
}
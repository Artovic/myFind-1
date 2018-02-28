//*
// @file myfind.c
// Betriebssysteme Abgabe1 myFind.
// Beispiel 1
//
// @author Dominic Mages <ic17b014@technikum-wien.at>
// @date 2005/02/22
//
// @version 001
//
// @todo Add requried fucntionality.
//

#include <errno.h>
// #include <error.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    // prevent warnings regarding unused params
    argc = argc;
    // argv = argv;

    char *sPath = argv[1];
    char cwd[] = "./";

    if (sPath == NULL) {
        sPath = cwd;
    }

    fprintf(stdout, "Path is: %s\n", sPath);

    // if (printf("Hello world!\n") < 0) {
    // 	error(1, errno, "printf() failed");
    // }
    // if (fflush(NULL) == EOF) {
    // 	error(1, errno, "fflush() failed");
    // }
    return 0;
}
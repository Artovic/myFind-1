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
// @todo Add required fucntionality.
//

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h> // strerror

// Prototypes
void do_file(char *fp_path, struct stat atrr);
void do_dir(char *dp_path, struct stat sb);

int main(int argc, char *argv[])
{
    // prevent warnings regarding unused params
    argc = argc;
    // argv = argv;

    char *location = argv[1];
    char cwd[] = "./";
    struct stat attr;

    if (location == NULL)
    {
        location = cwd;
    }

    // debug
    // fprintf(stdout, "Path is: %s\n", location);

    errno = 0;
    if (lstat(location, &attr) == 0)
    {
        do_file(location, attr);
        if (S_ISDIR(attr.st_mode))
        {
            do_dir(location, attr);
        }
    }
    else
    {
        fprintf(stderr, "myfind: lstat(%s): %s\n", location, strerror(errno));
    }

    return 0;
}

// do_file - simple in the beginning, will grow enormeous
void do_file(char *fp_path, struct stat attr)
{
    attr = attr; //prevent unused params, need for later implementation
    printf("%s\n", fp_path);
}

// do_dir - main logic for intermediate showcase
void do_dir(char *dpath, struct stat sb)
{
    DIR *dir;
    struct dirent *entry;
    sb = sb;
    char *slash = "";
    char *fullpath;
    unsigned long pathlength;

    errno = 0;
    dir = opendir(dpath);

    if (!dir)
    {
        fprintf(stderr, "myfind: opendir(%s): %s\n", dpath, strerror(errno));
        return;
    }

    while ((entry = readdir(dir)))
    {
        // skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        pathlength = strlen(dpath);

        // add trailing slash if not present
        if (dpath[pathlength - 1] != '/')
        {
            slash = "/";
        }

        /* allocate memory for the full entry path */
        pathlength += strlen(entry->d_name) + 2;
        fullpath = malloc(sizeof(char) * pathlength);

        if (!fullpath)
        {
            fprintf(stderr, "myfind: malloc(): %s\n", strerror(errno));
            break; // a return would require a closedir()
        }

        /* concat the path with the entry name */
        if (snprintf(fullpath, pathlength, "%s%s%s", dpath, slash, entry->d_name) < 0)
        {
            fprintf(stderr, "myfind: snprintf(): %s\n", strerror(errno));
            free(fullpath);
            break;
        }

        if (lstat(fullpath, &sb) == 0)
        {
            do_file(fullpath, sb);
            //if a directory, call the function recursively
            if (S_ISDIR(sb.st_mode))
            {
                do_dir(fullpath, sb);
            }
        }
        else
        {
            fprintf(stderr, "myfind: lstat(%s): %s\n", fullpath, strerror(errno));
            free(fullpath);
            continue;
        }

        free(fullpath);
    }

    if (closedir(dir) != 0)
    {
        fprintf(stderr, "myfind: closedir(%s): %s\n", dpath, strerror(errno));
        return;
    }
}
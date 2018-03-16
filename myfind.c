//*
// @file myfind.c
// Betriebssysteme Abgabe1 myFind.
// Beispiel 1
//
// @author Dominic Mages <ic17b014@technikum-wien.at>
// @author Ralph Hödl <ic17b003@technikum-wien.at>
// @author David North <ic17b086@technikum-wien.at>
// @date 2018/03/17
//
// @version 002
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
void do_file(const char *fp_path, const struct stat *atrr);
void do_dir(const char *dp_path, const struct stat *sb);

int main(int argc, char *argv[])
{
    // prevent warnings regarding unused params
    argc = argc;
    // argv = argv;

    char *location = argv[1];
    struct stat *attr = NULL;

//TODO: lstat is ueberflussig --> da in do_file passiert
//TODO: in main only do_file, rest handled in those functions
        do_file(location, attr);
//TODO: argv[0] anstatt hardcoded filename

    return 0;
}

// TODO: aParams - to mangage all params including filename and targetdirectory
// do_file - simple in the beginning, will grow enormeous
void do_file(const char *fp_path, const struct stat *attr)
{
    errno = 0;
    if(lstat(fp_path, attr) == 0) {
	
    }
//TODO: erster entry kann file oder directory sein und muss vorhanden sein
//TODO: lstat hier, wenn nicht --> exit code

//TODO: if (S_ISDIR(sb.st_mode)) check in do_file. if true, call do_dir, if falsy, countinue do_file 
    attr = attr; //prevent unused params, need for later implementation
    //if(lstat(fp_path, attr) != 0) {
    //    fprintf(stderr, "here is my awesome argv[0]: lstat(%s): %s\n", fullpath, strerror(errno));
    //}
    //if(S_ISDIR(attr->st_mode)) {
//	do_dir(fp_path, 
  //  }
    printf("%s\n", fp_path);
//    return 0;
}

// do_dir - main logic for intermediate showcase
void do_dir(const char *dpath, const struct stat *sb)
{
    DIR *dir;
    struct dirent *entry;
    char *slash = "";
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
        char fullpath[pathlength];

        /* concat the path with the entry name */
        if (snprintf(fullpath, pathlength, "%s%s%s", dpath, slash, entry->d_name) < 0)
        {
            fprintf(stderr, "myfind: snprintf(): %s\n", strerror(errno));
            break;
        }
        do_file(fullpath, sb);
    }

    if (errno != 0) {
	fprintf(stderr, "my fucking awesome argv[0] replacement: readdir(%s): %s\n", dpath, strerror(errno));
    }
    if (closedir(dir) != 0)
    {
        fprintf(stderr, "myfind: closedir(%s): %s\n", dpath, strerror(errno));
        return;
    }
}

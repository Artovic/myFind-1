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

#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// linked list that contains all params
typedef struct s_params
{
    char *location;
    int print;
    int ls;
    int nouser;
    char type;
    char *user;
    unsigned int userid;
    char *path;
    char *name;
    struct s_params *next;
} t_params;

// global for program name - got ok from galla for this
char *programName = "";

// Prototypes
int parse_params(int argc, char *argv[], t_params *params);
int free_params(t_params *params);
void do_file(const char *fp_path, t_params *params);
void do_dir(const char *dp_path, t_params *params);
int do_user(unsigned int userid, struct stat *attr);
int do_path(char *path, char *pattern);
int do_name(char *path, char *pattern);
int do_type(char type, struct stat *attr);
int do_print(const char *fp_path);
int do_ls(char *path, struct stat *attr);
// int do_nouser(const stat *attr);

int main(int argc, char *argv[])
{
    t_params *params;
    programName = argv[0];
    // prevent warnings regarding unused params
    argc = argc;
    // argv = argv;

    //
    params = calloc(1, sizeof(*params));
    if (!params)
    {
        fprintf(stderr, "%s: calloc(): %s\n", programName, strerror(errno));
        return 1;
    }

    if (parse_params(argc, argv, params) != 0)
    {
        free_params(params);
        return 1;
    }

    char *location = argv[1];

    do_file(location, params);

    free_params(params);
    return 0;
}

// TODO: aParams - to mangage all params including filename and targetdirectory
// @ralph, david: Hab das zurückgeändert in struct stat *attr, weil ich nicht verstehe, wie das mit const funktionieren soll
// ls stat verweigert eine verarbeitung mit const... ka was der galla da dann will....
void do_file(const char *fp_path, t_params *params)
{
    struct stat attr;

    errno = 0;
    if (lstat(fp_path, &attr) == 0)
    {
        // printf("%s\n", fp_path);
        if (S_ISDIR(attr.st_mode))
        {
            do_dir(fp_path, params);
        }
    }
    //TODO: erster entry kann file oder directory sein und muss vorhanden sein
    //TODO: lstat hier, wenn nicht --> exit code

    //TODO: if (S_ISDIR(sb.st_mode)) check in do_file. if true, call do_dir, if falsy, countinue do_file
    else
    {
        fprintf(stderr, "%s: lstat(%s): %s\n", programName, fp_path, strerror(errno));
        }
    //    return 0;
}

// do_dir - main logic for intermediate showcase
void do_dir(const char *dpath, t_params *params)
{
    DIR *dir;
    struct dirent *entry;
    char *slash = "";
    unsigned long pathlength;

    errno = 0;
    dir = opendir(dpath);

    if (!dir)
    {
        fprintf(stderr, "%s: opendir(%s): %s\n", programName, dpath, strerror(errno));
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
            fprintf(stderr, "%s: snprintf(): %s\n", programName, strerror(errno));
            break;
        }
        do_file(fullpath, params);
    }

    if (errno != 0)
    {
        fprintf(stderr, "%s: readdir(%s): %s\n", programName, dpath, strerror(errno));
    }
    if (closedir(dir) != 0)
    {
        fprintf(stderr, "%s: closedir(%s): %s\n", programName, dpath, strerror(errno));
        return;
    }
}

int parse_params(int argc, char *argv[], t_params *params)
{
    struct passwd *pwd;
    int i; /* used outside of the loop */

    int status = 0;
    int expression = 0;

    /* params can start from argv[1] */
    for (i = 1; i < argc; i++, params = params->next)
    {

        /* allocate memory for the next run */
        params->next = calloc(1, sizeof(*params));

        if (!params->next)
        {
            fprintf(stderr, "%s: calloc(): %s\n", programName, strerror(errno));
            return 1;
        }

        /* parameters consisting of a single part */
        if (strcmp(argv[i], "-print") == 0)
        {
            params->print = 1;
            expression = 1;
            continue;
        }
        if (strcmp(argv[i], "-ls") == 0)
        {
            params->ls = 1;
            expression = 1;
            continue;
        }
        if (strcmp(argv[i], "-nouser") == 0)
        {
            params->nouser = 1;
            expression = 1;
            continue;
        }

        /* parameters expecting a non-empty second part */
        if (strcmp(argv[i], "-user") == 0)
        {
            if (argv[++i])
            {
                params->user = argv[i];
                /* check if the user exists */
                if ((pwd = getpwnam(params->user)))
                {
                    params->userid = pwd->pw_uid;
                    expression = 1;
                    continue;
                }
                /* otherwise, if the input is a number, use that */
                if (sscanf(params->user, "%u", &params->userid))
                {
                    expression = 1;
                    continue;
                }
                status = 4;
                break; /* the user is not found and not a number */
            }
            else
            {
                status = 2;
                break; /* the second part is missing */
            }
        }
        if (strcmp(argv[i], "-name") == 0)
        {
            if (argv[++i])
            {
                params->name = argv[i];
                expression = 1;
                continue;
            }
            else
            {
                status = 2;
                break; /* the second part is missing */
            }
        }
        if (strcmp(argv[i], "-path") == 0)
        {
            if (argv[++i])
            {
                params->path = argv[i];
                expression = 1;
                continue;
            }
            else
            {
                status = 2;
                break; /* the second part is missing */
            }
        }

        /* a parameter expecting a restricted second part */
        if (strcmp(argv[i], "-type") == 0)
        {
            if (argv[++i])
            {
                if ((strcmp(argv[i], "b") == 0) || (strcmp(argv[i], "c") == 0) ||
                    (strcmp(argv[i], "d") == 0) || (strcmp(argv[i], "p") == 0) ||
                    (strcmp(argv[i], "f") == 0) || (strcmp(argv[i], "l") == 0) ||
                    (strcmp(argv[i], "s") == 0))
                {
                    params->type = argv[i][0];
                    expression = 1;
                    continue;
                }
                else
                {
                    status = 3;
                    break; /* the second part is unknown */
                }
            }
            else
            {
                status = 2;
                break; /* the second part is missing */
            }
        }

        /*
     * there was no match;
     * if the parameter starts with '-', return an error,
     * else if there were no previous matches (expressions), save it as a location
     */
        if (argv[i][0] == '-')
        {
            status = 1;
            break;
        }
        else
        {
            if (expression == 0)
            {
                params->location = argv[i];
                continue;
            }
            else
            {
                status = 5;
                break;
            }
        }
    }

    /* error handling */
    if (status == 1)
    {
        fprintf(stderr, "%s: unknown predicate: `%s'\n", programName, argv[i]);
        return 1;
    }
    if (status == 2)
    {
        fprintf(stderr, "%s: missing argument to `%s'\n", programName, argv[i - 1]);
        return 1;
    }
    if (status == 3)
    {
        fprintf(stderr, "%s: unknown argument to %s: %s\n", programName, argv[i - 1], argv[i]);
        return 1;
    }
    if (status == 4)
    {
        fprintf(stderr, "%s: `%s' is not the name of a known user\n", programName, argv[i]);
        return 1;
    }
    if (status == 5)
    {
        fprintf(stderr, "%s: paths must precede expression: %s\n", programName, argv[i]);
        fprintf(stderr, "Usage: %s <location> [ <aktion> ]\n", programName);
        return 1;
    }

    return 0;
}

int free_params(t_params *params)
{

    while (params)
    {
        t_params *next = params->next;
        free(params);
        params = next;
    }

    return 1;
}
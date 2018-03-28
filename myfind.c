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
// @todo fix ls
// @todo check type - f not working
// @todo check name and path again
// @todo fix param cycling and logic
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
#include <stdbool.h>

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
int do_startPoint(char *fp_path, t_params *params);
int do_file(char *fp_path, t_params *params, struct stat attr);
void do_dir(char *dp_path, t_params *params, struct stat attr);
int do_user(unsigned int userid, struct stat attr);
int do_type(char type, struct stat attr);
int do_ls(char *path, struct stat attr);
int do_name(char *path, char *pattern);
int do_path(char *path, char *pattern);
int do_print(char *fp_path);
// int do_nouser(const stat *attr);

int main(int argc, char *argv[])
{
    t_params *params;
    programName = argv[0];

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

    if (do_startPoint(location, params) != 0)
    {
        free_params(params);
        return 1;
    }

    free_params(params);

    return 0;
}

// helper function that allows us to abort a file with return
int do_startPoint(char *startPoint, t_params *params)
{
    errno = 0;
    struct stat attr;

    if (lstat(startPoint, &attr) == 0)
    {
        do_file(startPoint, params, attr);
        /* if a directory, process its contents */
        if (S_ISDIR(attr.st_mode))
        {
            do_dir(startPoint, params, attr);
        }
    }
    else
    {
        fprintf(stderr, "%s: lstat(%s): %s\n", programName, startPoint, strerror(errno));
    }

    if (errno != 0)
    {
        return 1;
    }
    return 0;
}

// TODO: aParams - to mangage all params including filename and targetdirectory
// @ralph, david: Hab das zurückgeändert in struct stat *attr, weil ich nicht verstehe, wie das mit const funktionieren soll
int do_file(char *fp_path, t_params *params, struct stat attr)
{
    // char *f = basename(fp_path);
    errno = 0;

    int wasPrinted = 0;
    wasPrinted = wasPrinted;
    // starting with filter functionalities
    // type
    if (params->type)
    {
        if (do_type(params->type, attr) != 0)
        {
            return 0; /* the entry didn't pass the check, do not print it */
        }
    }

    // user
    if (params->user)
    {
        if (do_user(params->userid, attr) != 0)
        {
            return 0;
        }
    }

    if (params->name)
    {
        if (do_name(fp_path, params->name) != 0)
        {
            return 0;
        }
    }

    if (params->path)
    {
        if (do_path(fp_path, params->path) != 0)
        {
            return 0;
        }
    }

    // print functionality
    // simple path print
    if (params->print)
    {
        if (do_print(fp_path) != 0)
        {
            // return 1; /* a fatal error occurred */
        }
        wasPrinted = 1;
    }

    // detailed ls print
    if (params->ls)
    {
        if (do_ls(fp_path, attr) != 0)
        {
            return 1;
        }
        wasPrinted = 1;
    }

    if (wasPrinted == 0)
    {
        if (do_print(fp_path) != 0)
        {
            return 1;
        }
    }

    return true;
}

// do_dir - main logic for intermediate showcase
void do_dir(char *dpath, t_params *params, struct stat attr)
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
        if (lstat(fullpath, &attr) == 0)
        {
            do_file(fullpath, params, attr);
            if (S_ISDIR(attr.st_mode))
            {
                do_dir(fullpath, params, attr);
            }
        }
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
    int i; // used outside of the loop

    int status = 0;

    // params can start from argv[2] because 0 is progname and 1 is path
    for (i = 2; i < argc; i++, params = params->next)
    {

        // allocate memory for the next run
        params->next = calloc(1, sizeof(*params));

        if (!params->next)
        {
            fprintf(stderr, "%s: calloc(): %s\n", programName, strerror(errno));
            return 1;
        }

        // single part params
        if (strcmp(argv[i], "-print") == 0)
        {
            params->print = 1;
            continue;
        }
        if (strcmp(argv[i], "-ls") == 0)
        {
            params->ls = 1;
            continue;
        }
        if (strcmp(argv[i], "-nouser") == 0)
        {
            params->nouser = 1;
            continue;
        }

        // params that require a second part
        if (strcmp(argv[i], "-user") == 0)
        {
            if (argv[++i])
            {
                params->user = argv[i];
                // check if user exists
                if ((pwd = getpwnam(params->user)))
                {
                    params->userid = pwd->pw_uid;
                    continue;
                }
                // or if the value is a number
                if (sscanf(params->user, "%u", &params->userid))
                {
                    continue;
                }
                status = 4;
                break; // user is wether a number nor existing
            }
            else
            {
                status = 2;
                break; // value is missing
            }
        }
        if (strcmp(argv[i], "-name") == 0)
        {
            if (argv[++i])
            {
                params->name = argv[i];
                continue;
            }
            else
            {
                status = 2;
                break; // value is missing
            }
        }
        if (strcmp(argv[i], "-path") == 0)
        {
            if (argv[++i])
            {
                params->path = argv[i];
                continue;
            }
            else
            {
                status = 2;
                break; // value is missing
            }
        }

        // params that expects a second part that can only be an existing file descriptor
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
                    continue;
                }
                else
                {
                    status = 3;
                    break; // value is not a valid file descriptor
                }
            }
            else
            {
                status = 2;
                break; // value is missing
            }
        }

        // handler for params that do not exist
        if (argv[i][0] == '-')
        {
            status = 1;
            break;
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
        fprintf(stderr, "Usage: %s <file or directory> [ <action> ]\n", programName);
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

int do_print(char *fp_path)
{

    if (printf("%s\n", fp_path) < 0)
    {
        fprintf(stderr, "%s: printf(): %s\n", programName, strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int do_path(char *fullpath, char *pattern)
{
    errno = 0;
    int flags = 0;

    if (fnmatch(pattern, fullpath, flags) == 0)
    {
        return 0;
    }
    if (errno != 0)
    {
        fprintf(stderr, "%s: fnmatch(%s): %s\n", programName, fullpath, strerror(errno));
        return 1;
    }

    // if not matching or error 1 shall be returned
    return 1;
}

int do_name(char *fp_path, char *pattern)
{
    errno = 0;
    char *filename = basename(fp_path);
    int flags = 0;

    if (fnmatch(pattern, filename, flags) == 0)
    {
        return 0;
    }
    if (errno != 0)
    {
        fprintf(stderr, "%s: fnmatch(%s): %s\n", programName, filename, strerror(errno));
    }

    // if not matching or error 1 shall be returned
    return 1;
}

int do_user(unsigned int userid, struct stat attr)
{
    if (userid == (unsigned int)attr.st_uid)
        return 0;
    else
        return 1;
}

int do_type(char type, struct stat attr)
{
    int checkvalue;
    switch (type)
    {
    case 'd':
        checkvalue = S_ISDIR(attr.st_mode);
        break;
    case 'b':
        checkvalue = S_ISBLK(attr.st_mode);
        break;
    case 'c':
        checkvalue = S_ISCHR(attr.st_mode);
        break;
    case 'p':
        checkvalue = S_ISFIFO(attr.st_mode);
        break;
    case 'f':
        checkvalue = S_ISREG(attr.st_mode);
        break;
    case 'l':
        checkvalue = S_ISLNK(attr.st_mode);
        break;
    case 's':
        checkvalue = S_ISSOCK(attr.st_mode);
        break;
    default:
        checkvalue = 0;
        break;
    }

    if (checkvalue != 0)
        return 0;
    else
        return 1;
}

int do_ls(char *path, struct stat attr)
{
    //-rwxrwxrwx 1 root root 1234 March 17 10:00 filename.exention
    int i = 0;
    char typ = '0';
    char *user;
    char *group;
    struct passwd *pwd;
    struct group *grp;
    char *stime = "";
    time_t t = time(NULL);
    struct tm *deadline_time = localtime(&t);
    if (deadline_time == NULL)
        return 1;
    deadline_time->tm_year = deadline_time->tm_year + 1900;
    struct tm *modified_time = localtime(&attr.st_mtime);
    if (modified_time == NULL)
        return 1;
    modified_time->tm_year = modified_time->tm_year + 1900;
    int flag = 1;
    char *filename;
    char *symlink;
    char *pfeil;

    //user and group name
    grp = getgrgid((unsigned int)attr.st_gid);
    if (grp == NULL)
        return 1;
    group = grp->gr_name;

    pwd = getpwuid((unsigned int)attr.st_uid);
    if (pwd == NULL)
        user = group;
    else
        user = pwd->pw_name;

    //actual date - 6 Months
    deadline_time->tm_mon = deadline_time->tm_mon - 6;
    if (deadline_time->tm_mon < 0)
    {
        deadline_time->tm_mon = deadline_time->tm_mon + 12;
        deadline_time->tm_year = deadline_time->tm_year - 1;
    }
    if ((deadline_time->tm_mon == 3 || deadline_time->tm_mon == 5 || deadline_time->tm_mon == 8 || deadline_time->tm_mon == 10) && deadline_time->tm_mday == 31) //check if 31st in April, June, September, November
    {
        deadline_time->tm_mday = 30;
    }
    if (deadline_time->tm_mon == 1 && deadline_time->tm_mday > 28) //February (28 or 29 days)
    {
        if (deadline_time->tm_year % 100 == 0)
        {
            if (deadline_time->tm_year % 400 == 0 && deadline_time->tm_mday > 29)
            { //leapyear
                deadline_time->tm_mday = 29;
            }
            else
            { //no leapyear
                deadline_time->tm_mday = 28;
            }
        }
        else
        {
            if (deadline_time->tm_year % 4 == 0 && deadline_time->tm_mday > 29)
            { //leapyear
                deadline_time->tm_mday = 29;
            }
            else
            { //no leapyear
                deadline_time->tm_mday = 28;
            }
        }
    }

    //check if last modification is older than 6 months -> if true, display year instead of time
    if (deadline_time->tm_year < modified_time->tm_year)
        flag = 1; //year deadline_time < year modified_time = show time
    else if (deadline_time->tm_year == modified_time->tm_year)
    {
        if (deadline_time->tm_mon < modified_time->tm_mon)
            flag = 1; //years equal, month deadline_time < month modified_time = show time
        else if (deadline_time->tm_mon == modified_time->tm_mon)
        {
            if (deadline_time->tm_mday <= modified_time->tm_mday)
                flag = 1; // years equal, months equal, day deadline_time <= day modified_time = show time
            else
                flag = 0; // years equal, months equal, day deadline_time > day modified_time = show year
        }
        else
            flag = 0; //years equal, month deadline_time > month modified_time = show year
    }
    else
    {
        flag = 0; //year deadline_time > year modified_time = show year
    }

    if (flag == 1)
    {
        if (strftime(stime, 36, "%b %d %H:%S", localtime(&attr.st_mtime)) == 0)
            return 1;
    }
    else
    {
        if (strftime(stime, 36, "%b %d %Y", localtime(&attr.st_mtime)) == 0)
            return 1;
    }

    //type of file
    while (i < 7 && typ == '0')
    {
        switch (i)
        {
        case 0:
            (S_ISDIR(attr.st_mode)) ? (typ = 'd') : (0);
            break;
        case 1:
            (S_ISBLK(attr.st_mode)) ? (typ = 'b') : (0);
            break;
        case 2:
            (S_ISCHR(attr.st_mode)) ? (typ = 'c') : (0);
            break;
        case 3:
            (S_ISFIFO(attr.st_mode)) ? (typ = '-') : (0);
            break;
        case 4:
            (S_ISREG(attr.st_mode)) ? (typ = 'f') : (0);
            break;
        case 5:
            (S_ISLNK(attr.st_mode)) ? (typ = 'l') : (0);
            break;
        case 6:
            (S_ISSOCK(attr.st_mode)) ? (typ = 's') : (0);
            break;
        }
    }
    if (typ == 0)
        return 1;

    //filename
    if (typ == 'd')
        filename = dirname(path);
    else
        filename = basename(path);

    //symbolic link
    if (typ == 'l')
    {
        errno = 0;
        ssize_t length;
        length = readlink(filename, symlink, attr.st_size + 1);
        if (length < 0)
        {
            fprintf(stderr, "%s: readlink(%s): %s\n", programName, filename, strerror(errno));
            free(symlink);
            return 1;
        }
        if (errno != 0)
        {
            return 1;
        }
        symlink[attr.st_size] = '\0';
        pfeil = " -> ";
    }
    else
    {
        symlink = "";
        pfeil = "";
    }

    //print
    printf("%c%c%c%c%c%c%c%c%c%c %d %s %s %ld %s %s %s %s",
           typ,
           (S_IRUSR & attr.st_mode) ? ('r') : ('-'),
           (S_IWUSR & attr.st_mode) ? ('w') : ('-'),
           (S_ISUID & attr.st_mode) ? ((S_IXUSR & attr.st_mode) ? ('s') : ('S')) : ((S_IXUSR & attr.st_mode) ? ('x') : ('-')),
           (S_IRGRP & attr.st_mode) ? ('r') : ('-'),
           (S_IWGRP & attr.st_mode) ? ('w') : ('-'),
           (S_ISGID & attr.st_mode) ? ((S_IXGRP & attr.st_mode) ? ('s') : ('S')) : ((S_IXGRP & attr.st_mode) ? ('x') : ('-')),
           (S_IROTH & attr.st_mode) ? ('r') : ('-'),
           (S_IWOTH & attr.st_mode) ? ('w') : ('-'),
           (S_ISVTX & attr.st_mode) ? ((S_IXOTH & attr.st_mode) ? ('t') : ('T')) : ((S_IXOTH & attr.st_mode) ? ('x') : ('-')),
           (unsigned int)attr.st_nlink,
           user,
           group,
           attr.st_blocks,
           stime,
           filename,
           pfeil,
           symlink);

    return 0;
}
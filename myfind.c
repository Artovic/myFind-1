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
// @version 003
//
// @todo fix ls
// @todo fix param cycling and logic and more params to be set
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
#include <time.h>

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
int do_nouser(struct stat attr);
int do_type(char type, struct stat attr);
int do_ls(char *path, struct stat attr);
int do_name(char *path, char *pattern);
int do_path(char *path, char *pattern);
int do_print(char *fp_path);

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

int do_file(char *fp_path, t_params *params, struct stat attr)
{
    int wasPrinted = 0;

    do
    {

        // starting with filter functionalities
        // type
        if (params->type)
        {
            if (do_type(params->type, attr) != 0)
                return 0; /* the entry didn't pass the check, do not print it */
        }

        // user
        if (params->user)
        {
            if (do_user(params->userid, attr) != 0)
                return 0;
        }

        if (params->nouser)
        {
            if (do_nouser(attr) != 0)
                return 0;
        }

        if (params->name)
        {
            if (do_name(fp_path, params->name) != 0)
                return 0;
        }

        if (params->path)
        {
            if (do_path(fp_path, params->path) != 0)
                return 0;
        }

        // print functionality
        // simple path print
        if (params->print)
        {
            if (do_print(fp_path) != 0)
            {
                return 1; // fatal error
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

        params = params->next;
    } while (params);

    if (wasPrinted == 0)
    {
        if (do_print(fp_path) != 0)
        {
            return 1;
        }
    }

    return true;
}

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

int do_nouser(struct stat attr)
{
    static unsigned int cache_uid = UINT_MAX;

    // skip getgrgid if we have the record in cache
    if (cache_uid == attr.st_uid)
    {
        return 1;
    }

    // reset cache
    cache_uid = UINT_MAX;

    if (!getpwuid(attr.st_uid))
    {
        return 0;
    }

    // cache an existing user (more common)
    cache_uid = attr.st_uid;

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
    errno = 0;
    unsigned long inode = attr.st_ino;
    long long blocks = S_ISLNK(attr.st_mode) ? 0 : attr.st_blocks / 2;
    unsigned long links = attr.st_nlink;
    long long size = attr.st_size;
    char *user;
    char *group;
    struct passwd *pwd;
    struct group *grp;
    char *symlink = NULL;
    ssize_t length;
    ssize_t buffersize;

    grp = getgrgid((unsigned int)attr.st_gid);
    if (grp == NULL)
        return 1;
    group = grp->gr_name;

    pwd = getpwuid((unsigned int)attr.st_uid);
    if (pwd == NULL)
        user = group;
    else
        user = pwd->pw_name;

    char *mtime = ctime(&attr.st_mtim.tv_sec) + 4;
    mtime[strlen(mtime) - 9] = '\0';

    buffersize = attr.st_size + 1;

    if (S_ISLNK(attr.st_mode))
    {
        if (attr.st_size == 0)
            buffersize = PATH_MAX;

        symlink = malloc(sizeof(char) * buffersize);
        if (symlink == NULL)
        {
            fprintf(stderr, "%s: malloc(): Not enough memory to continue\n", programName);
            exit(1);
        }

        while ((length = readlink(path, symlink, buffersize)) > 1 && (length > buffersize))
        {
            buffersize *= 2;
            if ((symlink = realloc(symlink, sizeof(char) * buffersize)) == NULL)
            {
                free(symlink);
                fprintf(stderr, "%s: malloc(): Not enough memory to continue\n", programName);
                exit(1);
            }
        }
        if (length == -1)
        {
            free(symlink);
            fprintf(stderr, "%s: readlink()\n", programName);
            exit(1);
        }

        symlink[length] = '\0';
    }
    else
    {
        symlink = NULL;
    }

    static char permissions[11];
    char filetype;
    path = path;

    switch (attr.st_mode & S_IFMT)
    {
    case S_IFREG:
        filetype = '-';
        break;
    case S_IFDIR:
        filetype = 'd';
        break;
    case S_IFBLK:
        filetype = 'b';
        break;
    case S_IFCHR:
        filetype = 'c';
        break;
    case S_IFIFO:
        filetype = 'p';
        break;
    case S_IFLNK:
        filetype = 'l';
        break;
    case S_IFSOCK:
        filetype = 's';
        break;
    default:
        filetype = '?';
    }

    permissions[0] = (char)(filetype == 'f' ? '-' : filetype);
    permissions[1] = (char)(attr.st_mode & S_IRUSR ? 'r' : '-');
    permissions[2] = (char)(attr.st_mode & S_IWUSR ? 'w' : '-');
    permissions[3] = (char)(attr.st_mode & S_ISUID ? (attr.st_mode & S_IXUSR ? 's' : 'S')
                                                   : (attr.st_mode & S_IXUSR ? 'x' : '-'));
    permissions[4] = (char)(attr.st_mode & S_IRGRP ? 'r' : '-');
    permissions[5] = (char)(attr.st_mode & S_IWGRP ? 'w' : '-');
    permissions[6] = (char)(attr.st_mode & S_ISGID ? (attr.st_mode & S_IXGRP ? 's' : 'S')
                                                   : (attr.st_mode & S_IXGRP ? 'x' : '-'));
    permissions[7] = (char)(attr.st_mode & S_IROTH ? 'r' : '-');
    permissions[8] = (char)(attr.st_mode & S_IWOTH ? 'w' : '-');
    permissions[9] = (char)(attr.st_mode & S_ISVTX ? (attr.st_mode & S_IXOTH ? 't' : 'T')
                                                   : (attr.st_mode & S_IXOTH ? 'x' : '-'));
    permissions[10] = '\0';

    fprintf(stdout, "%18lu %4lld %10s %3lu %-8s %-8s %8lld %12s %s%s%s\n",
            inode, blocks, permissions, links, user, group, size, mtime, path, (symlink ? "->" : ""), (symlink ? symlink : ""));

    free(symlink);
    return 0;
}
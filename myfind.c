//*
// @file myfind.c
// Betriebssysteme Abgabe1 myFind.
// Beispiel 1
//
// @author Dominic Mages <ic17b014@technikum-wien.at>
// @author Ralph Hödl <ic17b003@technikum-wien.at>
// @author David North <ic17b086@technikum-wien.at>
// @date 2018/04/05
//
// @version 007
//
//

// -------------------------------------------------------------- includes --
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
#include <time.h>
#include <ctype.h>

// --------------------------------------------------------------- defines --

// -------------------------------------------------------------- typedefs --
// linked list that contains parsed params
typedef struct s_params
{
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

// --------------------------------------------------------------- globals --
// global for program name - got ok from galla for this
char *programName = "";

// ------------------------------------------------------------- functions --
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

//*
// \brief Reimplementation of GNU find
//
// This is the main entrypoint
//
// \param argc the number of arguments
// \param argv the arguments itselves (including the program name in argv[0])
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
int main(int argc, char *argv[])
{
    t_params *params;
    programName = argv[0];

    params = calloc(1, sizeof(*params));
    if (!params)
    {
        fprintf(stderr, "%s: calloc(): %s\n", programName, strerror(errno));
        return EXIT_FAILURE;
    }

    if (parse_params(argc, argv, params) != 0)
    {
        free_params(params);
        return EXIT_FAILURE;
    }

    char *location = argv[1];

    if (do_startPoint(location, params) != 0)
    {
        free_params(params);
        return EXIT_FAILURE;
    }

    free_params(params);

    return EXIT_SUCCESS;
}

//*
// \brief handles the starting location
//
// This function allows us to work with return values in do_file,
// which would not be possible if the do_file call is comming from main
//
// \param startPoint the command-line-given startpoint
// \param params the linked list of parsed parameters
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
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
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

//*
// \brief handler for a single file
//
// This function iterates over all passed parameters
// and filters for those.
// If the file does not meet the requirements, it will
// not be printed.
// If no print paramter was given, it will call do_print as fallback.
//
// \param fp_path path the file to be processed
// \param params the linked list of parsed parameters
// \param attr the entry attributes from lstat
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
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
                return EXIT_SUCCESS; /* the entry didn't pass the check, do not print it */
        }

        // user
        if (params->user)
        {
            if (do_user(params->userid, attr) != 0)
                return EXIT_SUCCESS;
        }

        if (params->nouser)
        {
            if (do_nouser(attr) != 0)
                return EXIT_SUCCESS;
        }

        if (params->name)
        {
            if (do_name(fp_path, params->name) != 0)
                return EXIT_SUCCESS;
        }

        if (params->path)
        {
            if (do_path(fp_path, params->path) != 0)
                return EXIT_SUCCESS;
        }

        // print functionality
        // simple path print
        if (params->print)
        {
            if (do_print(fp_path) != 0)
            {
                return EXIT_FAILURE; // fatal error
            }
            wasPrinted = 1;
        }

        // detailed ls print
        if (params->ls)
        {
            if (do_ls(fp_path, attr) != 0)
            {
                return EXIT_FAILURE;
            }
            wasPrinted = 1;
        }

        params = params->next;
    } while (params);

    if (wasPrinted == 0)
    {
        if (do_print(fp_path) != 0)
        {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

//*
// \brief handler for a single directory
//
// This function cotains all logic for a directory.
// If the directory does not exist, we will exit the function.
// It will skip . and .. and add a trailing slash if needed
// After the full path is created, it will call do file
// If the entry is a directory it will additionally call itself
//
// \param d_path path the file to be processed
// \param params the linked list of parsed parameters
// \param attr the entry attributes from lstat
//
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

//*
// \brief handler for parsing the command line params
//
// We will iterate over all command line arguments and
// map them to members of the current allocation of the
// params.
// In the same time a validation of those command line arguments
// is done.
// In case of error, a status will indicate, which error happened
// and print it.
//
// \param argc amount of arguments
// \param argv the arguments itself
// \param params the linked list of parsed parameters
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
int parse_params(int argc, char *argv[], t_params *params)
{
    struct passwd *pwd;
    int i; // used outside of the loop

    int status = 0;
    int expression = 0;

    // params can start from argv[2] because 0 is progname and 1 is path
    for (i = 2; i < argc; i++, params = params->next)
    {
        expression = 0;

        // allocate memory for the next run
        params->next = calloc(1, sizeof(*params));

        if (!params->next)
        {
            fprintf(stderr, "%s: calloc(): %s\n", programName, strerror(errno));
            return EXIT_FAILURE;
        }

        // single part params
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
                    expression = 1;

                    continue;
                }
                // or if the value is a number
                if (sscanf(params->user, "%u%*c", &params->userid))
                {
                    expression = 1;
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
                expression = 1;
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
                expression = 1;
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
                    expression = 1;
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

        if (expression == 0)
        {
            status = 1;
            break;
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
        fprintf(stderr, "%s: invalid predicate: `%s'\n", programName, argv[i]);
        return EXIT_FAILURE;
    }
    if (status == 2)
    {
        fprintf(stderr, "%s: missing argument to `%s'\n", programName, argv[i - 1]);
        return EXIT_FAILURE;
    }
    if (status == 3)
    {
        fprintf(stderr, "%s: unknown argument to %s: %s\n", programName, argv[i - 1], argv[i]);
        return EXIT_FAILURE;
    }
    if (status == 4)
    {
        fprintf(stderr, "%s: `%s' is not the name of a known user\n", programName, argv[i]);
        return EXIT_FAILURE;
    }
    if (status == 5)
    {
        fprintf(stderr, "%s: paths must precede expression: %s\n", programName, argv[i]);
        fprintf(stderr, "Usage: %s <file or directory> [ <action> ]\n", programName);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//*
// \brief handler for releasing the allocated memory of the params linked list
//
// Just iterate over the params until params is null
// in each iteration set params to the next params in the list
//
// \param params the linked list of parsed parameters
//
// \returns EXIT_SUCCESS
//
int free_params(t_params *params)
{

    while (params)
    {
        t_params *next = params->next;
        free(params);
        params = next;
    }

    return EXIT_SUCCESS;
}

//*
// \brief printing the current file path
//
// Just prints the passed parameter and has error handling
//
// \param fp_path path the file/directory to be processed
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
int do_print(char *fp_path)
{

    if (printf("%s\n", fp_path) < 0)
    {
        fprintf(stderr, "%s: printf(): %s\n", programName, strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//*
// \brief filter function for path
//
// checks the passed path against the passed pattern
//
// \param fullpath path the file/directory to be processed
// \param pattern pattern that shall match the path
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
int do_path(char *fullpath, char *pattern)
{
    errno = 0;
    int flags = 0;

    if (fnmatch(pattern, fullpath, flags) == 0)
    {
        return EXIT_SUCCESS;
    }
    if (errno != 0)
    {
        fprintf(stderr, "%s: fnmatch(%s): %s\n", programName, fullpath, strerror(errno));
        return EXIT_FAILURE;
    }

    // if not matching or error 1 shall be returned
    return EXIT_FAILURE;
}

//*
// \brief filter function for file-names
//
// checks the passed file-name against the passed pattern
//
// \param fp_path path the file/directory to be processed
// \param pattern pattern that shall match the path
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
int do_name(char *fp_path, char *pattern)
{
    errno = 0;
    char *filename = basename(fp_path);
    int flags = 0;

    if (fnmatch(pattern, filename, flags) == 0)
    {
        return EXIT_SUCCESS;
    }
    if (errno != 0)
    {
        fprintf(stderr, "%s: fnmatch(%s): %s\n", programName, filename, strerror(errno));
    }

    // if not matching or error 1 shall be returned
    return EXIT_FAILURE;
}

//*
// \brief filter function for users
//
// checks the passed userid against one in attr
//
// \param fp_path path the file/directory to be processed
// \param attr the entry attributes from lstat
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
int do_user(unsigned int userid, struct stat attr)
{
    if (userid == (unsigned int)attr.st_uid)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

//*
// \brief filter function for files without a user
//
// checks if the file has no userid
//
// \param attr the entry attributes from lstat
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
int do_nouser(struct stat attr)
{
    static unsigned int cache_uid = UINT_MAX;

    // skip getgrgid if we have the record in cache
    if (cache_uid == attr.st_uid)
    {
        return EXIT_FAILURE;
    }

    // reset cache
    cache_uid = UINT_MAX;

    if (!getpwuid(attr.st_uid))
    {
        return EXIT_SUCCESS;
    }

    // cache an existing user (more common)
    cache_uid = attr.st_uid;

    return EXIT_FAILURE;
}

//*
// \brief filter function the file type
//
// checks if the type matches the entry attributes
//
// \param type the file type from the params
// \param attr the entry attributes from lstat
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//
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
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

//*
// \brief prints the current file path with details
//
// Prints the passed path in a view
// with detailed information
//
// \param path path the file to be processed
// \param attr the entry attributes from lstat
//
// \returns EXIT_SUCCESS, EXIT_FAILURE
//

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
        return EXIT_FAILURE;
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
    return EXIT_SUCCESS;
}
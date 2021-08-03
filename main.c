
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "validators.h"

static bool SHOW_INODE = false;
static bool LONG_LIST_FORMAT = false;
static bool LIST_RECURSIVELY = false;
static bool DIR_PRINTED_ALTEAST_ONCE = false;
static bool MULTIPLE_DIR_REQUESTS = false;

const char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void getAndPrintGroup(const struct stat *buf) {
    if (!LONG_LIST_FORMAT) {
        return;
    }
    const gid_t grpNum = buf->st_gid;
    struct group *grp = NULL;
    grp = getgrgid(grpNum);

    printf("%s ", grp ? grp->gr_name : "-");
}

void getAndPrintUserName(const struct stat *buf) {
    if (!LONG_LIST_FORMAT) {
        return;
    }
    const uid_t uid = buf->st_uid;
    struct passwd *pw = NULL;
    pw = getpwuid(uid);

    printf("%s ", pw ? pw->pw_name : "-");
}

bool isValidFile(const struct dirent *file) {
    return (strncmp(file->d_name, ".", strlen(file->d_name)) != 0 &&
            strncmp(file->d_name, "..", strlen(file->d_name)) != 0 &&
            file->d_name[0] != '.');
}

// inspiration from
// https://stackoverflow.com/questions/10323060/printing-file-permissions-like-ls-l-using-stat2-in-c
void printPermissions(const struct stat *buf) {
    if (!LONG_LIST_FORMAT) {
        return;
    }
    char fileType = '-';
    const mode_t mode = buf->st_mode;
    if (S_ISDIR(mode)) {
        fileType = 'd';
    } else if (S_ISCHR(mode)) {
        fileType = 'c';
    } else if (S_ISLNK(mode)) {
        fileType = 'l';
    }
    printf("%c", fileType);
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
    printf(" ");
}

void printFileName(const struct dirent *file, bool isSymbolicLink,
                   const char *fullPath, const struct stat *buf) {
    printf("%s", file->d_name);
    if (isSymbolicLink && LONG_LIST_FORMAT) {
        // linkSize inspired from
        // https://man7.org/linux/man-pages/man2/readlink.2.html
        ssize_t linkSize = buf->st_size + 1;
        if (!buf->st_size) {
            linkSize = PATH_MAX;
        }
        char *link = calloc(linkSize, sizeof(char));
        ssize_t res = readlink(fullPath, link, linkSize);
        if (res == -1) {
            fprintf(stderr, "Value of errno: %d\n", errno);
            perror("Error printed by readlink");
        } else {
            printf(" -> %s", link);
        }

        free(link);
        link = NULL;
    }
    printf("\n");
}
void printFileType(const struct dirent *file) {
    printf("type:%u ", file->d_type);
}
void printFileLength(const struct dirent *file) {  // not file size
    if (LONG_LIST_FORMAT) {
        printf("%hu ", file->d_reclen);
    }
}
void printInode(const struct dirent *file) {
    if (SHOW_INODE) {
        printf("%6lu ", file->d_ino);
    }
}
void printPath(const char *path) { printf("%s:\n", path); }
void printRecursiveSpacer() {
    if (!DIR_PRINTED_ALTEAST_ONCE) {
        return;
    }
    printf("\n");
}
void printFileSize(const struct stat *buf) {
    if (!LONG_LIST_FORMAT) {
        return;
    }
    printf("%6ld ", buf->st_size);
}
void printNumOfHardLinks(const struct stat *buf) {
    if (!LONG_LIST_FORMAT) {
        return;
    }
    printf("%2ld ", buf->st_nlink);
}

void printLastModified(const struct stat *buf) {
    if (!LONG_LIST_FORMAT) {
        return;
    }
    const time_t *mtime = &buf->st_mtime;
    const struct tm *lastModified = localtime(mtime);
    printf("%3s %2d %04d %02d:%02d ", months[lastModified->tm_mon],
           lastModified->tm_mday, lastModified->tm_year + 1900,
           lastModified->tm_hour, lastModified->tm_min);
}

void printDir(const char *path) {
    DIR *pDir = validatePath(path);
    if (!pDir) {
        return;
    }
    const struct dirent *curr = readdir(pDir);
    if (LIST_RECURSIVELY || MULTIPLE_DIR_REQUESTS) {
        printRecursiveSpacer();
        DIR_PRINTED_ALTEAST_ONCE = true;
        printPath(path);
    }
    while (curr) {
        if (isValidFile(curr)) {
            unsigned int subDirLen = 0;
            subDirLen += strlen(path);
            subDirLen += strlen(curr->d_name);
            subDirLen += 3;  // ['/', '/', '\0']

            char *subDir = calloc(subDirLen, sizeof(char));
            const bool PathEndsWithSlash =
                strlen(path) && path[strlen(path) - 1] == '/';
            snprintf(subDir, subDirLen, PathEndsWithSlash ? "%s%s" : "%s/%s",
                     path, curr->d_name);

            struct stat *buf = malloc(sizeof(struct stat));
            if (lstat(subDir, buf) == -1) {
                fprintf(stderr, "stat() errno: %d for \"%s\"\n", errno, subDir);
            }
            printInode(curr);
            printPermissions(buf);
            printNumOfHardLinks(buf);
            getAndPrintUserName(buf);
            getAndPrintGroup(buf);
            // printFileLength(curr);
            // printFileType(curr);
            printFileSize(buf);
            printLastModified(buf);
            printFileName(curr, curr->d_type == 10, subDir, buf);
            free(subDir);
            free(buf);
            subDir = NULL;
            buf = NULL;
        }
        curr = readdir(pDir);
    }
    if (LIST_RECURSIVELY) {
        rewinddir(pDir);
        curr = readdir(pDir);
        while (curr) {
            if (isValidFile(curr)) {
                unsigned int subDirLen = 0;
                subDirLen += strlen(path);
                subDirLen += strlen(curr->d_name);
                subDirLen += 3;  // ['/', '/', '\0']

                char *subDir = calloc(subDirLen, sizeof(char));
                const bool PathEndsWithSlash =
                    strlen(path) && path[strlen(path) - 1] == '/';
                snprintf(subDir, subDirLen,
                         PathEndsWithSlash ? "%s%s" : "%s/%s", path,
                         curr->d_name);

                struct stat *buf = malloc(sizeof(struct stat));
                if (lstat(subDir, buf) == -1) {
                    fprintf(stderr,
                            "\nstat() errno: %d inside recursive option for "
                            "\"%s\"\n",
                            errno, subDir);
                    // perror("stat() error");
                }

                if (curr->d_type == 4) {
                    printDir(subDir);
                }
                free(subDir);
                free(buf);
                subDir = NULL;
                buf = NULL;
            }
            curr = readdir(pDir);
        }
    }
    closedir(pDir);
}

void setSelectedFlags(const char *flags) {
    if (!flags) {
        return;
    }
    const size_t len = strlen(flags);
    unsigned int i = 0;

    while (i != len) {
        if (flags[i] == 'i') {
            SHOW_INODE = true;
        } else if (flags[i] == 'l') {
            LONG_LIST_FORMAT = true;
        } else if (flags[i] == 'R') {
            LIST_RECURSIVELY = true;
        }
        i++;
    }
}

int main(int argc, char **argv) {
    // printf("argc: %d\n", argc);
    const char defaultPath[2] = {'.', '\0'};

    unsigned int i = 1;  // argv[0] always "./UnixLs"
    while (i < argc) {
        // printf("argv[%d]: %s\n", i, argv[i]);
        if (isFlagArg(argv[i])) {
            if (validateFlags(argv[i])) {
                setSelectedFlags(argv[i]);
            } else {
                exit(EXIT_FAILURE);
            }
        } else {
            break;
        }
        i++;
    }

    MULTIPLE_DIR_REQUESTS = argc - i > 1;

    unsigned int numOfDirArgs = 0;
    while (i < argc) {
        numOfDirArgs++;
        printDir(argv[i]);
        i++;
    }

    if (argc == 1 || !numOfDirArgs) {
        printDir(defaultPath);
    }

    return 0;
}

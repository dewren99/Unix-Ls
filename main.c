
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
    if (isSymbolicLink) {
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
        printf("%lu ", file->d_ino);
    }
}
void printPath(const char *path) { printf("%s:\n", path); }
void printRecursiveSpacer() { printf("\n"); }
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
    const char slash = '/';
    const struct dirent *curr = readdir(pDir);
    if (LIST_RECURSIVELY) {
        printRecursiveSpacer();
        printPath(path);
    }
    while (curr) {
        if (isValidFile(curr)) {
            unsigned int subDirLen = 0;
            subDirLen += strlen(path);
            subDirLen += strlen(&slash);  // accounts for the space for
                                          // slash and extra space for '\0'
            subDirLen += strlen(curr->d_name);

            char *subDir = calloc(subDirLen, sizeof(char));
            snprintf(subDir, subDirLen, "%s/%s", path, curr->d_name);

            struct stat *buf = malloc(sizeof(struct stat));
            if (lstat(subDir, buf) == -1) {
                fprintf(stderr, "stat() errno: %d for \"%s\"\n", errno, subDir);
                // perror("stat() error");
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
            if (LIST_RECURSIVELY && curr->d_type == 4) {
                printDir(subDir);
            }
            free(subDir);
            free(buf);
            subDir = NULL;
            buf = NULL;
        }
        curr = readdir(pDir);
    }
    closedir(pDir);
}

void setSelectedFlags(const char *flags) {
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
    const char defaultPath = '.';
    const char *flags = NULL;
    const char *path = NULL;

    if (argc > 3) {
        printf("Maximum argument number is 3, you've passed %d arguments\n",
               argc);
        return -1;
    }

    if (argc == 3) {
        path = argv[2];
        flags = argv[1];
    } else if (argc == 2) {
        // check if 2nd arg are flags, else its path
        if (strncmp(argv[1], "-", 1) == 0) {
            flags = argv[1];
        } else {
            path = argv[1];
        }
    } else {
        path = &defaultPath;
    }

    printf("argc: %d\n", argc);
    printf("flags: %s\n", flags);
    printf("path: %s\n", path);

    if (!validateFlags(flags) || !validatePath(path)) {
        return -1;
    }

    setSelectedFlags(flags);
    printDir(path);
    return 0;
}

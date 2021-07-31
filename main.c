
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "validators.h"

static bool SHOW_INODE = false;
static bool LONG_LIST_FORMAT = false;
static bool LIST_RECURSIVELY = false;

void getAndPrintGroup(gid_t grpNum) {
    struct group *grp;

    grp = getgrgid(grpNum);

    if (grp) {
        printf("The group ID %u -> %s\n", grpNum, grp->gr_name);
    } else {
        printf("No group name for %u found\n", grpNum);
    }
}

void getAndPrintUserName(uid_t uid) {
    struct passwd *pw = NULL;
    pw = getpwuid(uid);

    if (pw) {
        printf("The user ID %u -> %s\n", uid, pw->pw_name);
    } else {
        perror("Hmm not found???");
        printf("No name found for %u\n", uid);
    }
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
    const mode_t mode = buf->st_mode;
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void printFileName(const struct dirent *file) { printf("%s\n", file->d_name); }
void printFileType(const struct dirent *file) { printf("%u ", file->d_type); }
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
void printFileSize(const struct stat *buf) { printf("%ld ", buf->st_size); }
void printNumOfHardLinks(const struct stat *buf) {
    printf(" %ld ", buf->st_nlink);
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
            if (stat(subDir, buf) == -1) {
                fprintf(stderr, "stat() errno: %d\n", errno);
                perror("stat() error: ");
            }
            printInode(curr);
            printPermissions(buf);
            printNumOfHardLinks(buf);
            // printFileLength(curr);
            printFileType(curr);
            printFileSize(buf);
            printFileName(curr);
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

    printf("\n\n");

    // struct group *grp;

    // getAndPrintGroup(1001);
    // getAndPrintGroup(514378);
    // getAndPrintGroup(103);
    // getAndPrintGroup(1000);

    // getAndPrintUserName(59894);
    // getAndPrintUserName(23524);
    // getAndPrintUserName(20746);
    // getAndPrintUserName(5970);
    // getAndPrintUserName(10485);

    return 0;
}


#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void printFileName(const struct dirent *file) { printf("%s\n", file->d_name); }
void printFileType(const struct dirent *file) { printf("%u ", file->d_type); }
void printFileSize(const struct dirent *file) {
    printf("%hu ", file->d_reclen);
}
void printInode(const struct dirent *file) { printf("%lu ", file->d_ino); }
void printPath(const char *path) { printf("%s:\n", path); }

void printDir(const char *path) {
    DIR *pDir = validatePath(path);
    if (!pDir) {
        return;
    }
    const struct dirent *curr = readdir(pDir);
    if (LIST_RECURSIVELY) {
        printPath(path);
    }
    while (curr) {
        if (isValidFile(curr)) {
            if (SHOW_INODE) {
                printInode(curr);
            }
            if (LONG_LIST_FORMAT) {
                printFileSize(curr);
            }
            printFileType(curr);
            printFileName(curr);
            if (LIST_RECURSIVELY && curr->d_type == 4) {
                const char slash = '/';
                unsigned int subDirLen = 0;
                subDirLen += strlen(path);
                subDirLen += strlen(&slash);  // accounts for the space for
                                              // slash and extra space for '\0'
                subDirLen += strlen(curr->d_name);

                char *subDir = calloc(subDirLen, sizeof(char));
                snprintf(subDir, subDirLen, "%s/%s", path, curr->d_name);
                // printf("\n\nsubDir: %s\n\n", subDir);
                printDir(subDir);
                free(subDir);
                subDir = NULL;
            }
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

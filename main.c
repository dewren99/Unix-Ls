
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "validators.h"

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

void printDir(const char *path) {
    DIR *pDir = validatePath(path);
    if (!pDir) {
        return;
    }
    const struct dirent *curr = readdir(pDir);
    while (curr) {
        if (strncmp(curr->d_name, ".", strlen(curr->d_name)) != 0 &&
            strncmp(curr->d_name, "..", strlen(curr->d_name)) != 0) {
            printf("inode #: %lu, size: %hu, type: %u, name: %s\n", curr->d_ino,
                   curr->d_reclen, curr->d_type, curr->d_name);
        }
        curr = readdir(pDir);
    }
    closedir(pDir);
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

    printDir(path);

    printf("\n\n");

    struct group *grp;

    getAndPrintGroup(1001);
    getAndPrintGroup(514378);
    getAndPrintGroup(103);
    getAndPrintGroup(1000);

    getAndPrintUserName(59894);
    getAndPrintUserName(23524);
    getAndPrintUserName(20746);
    getAndPrintUserName(5970);
    getAndPrintUserName(10485);

    return 0;
}

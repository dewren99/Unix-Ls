#include "validators.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

bool validateFlags(const char *flags) {
    if (!flags) {
        return true;
    }
    size_t len = strlen(flags);
    if (len > 4) {
        printf(
            "Maximum expected number of flags are 3 excluding \"-\", got %zd\n",
            strlen(flags) - 1);
        return false;
    }
    if (strncmp(flags, "-", 1) != 0 || len < 2) {
        printf("Expected \"-\" before flag(s), got \"%s\"\n", flags);
        return false;
    }
    unsigned int i = 0;
    while (i != len) {
        if (i != 0 && flags[i] != 'i' && flags[i] != 'l' && flags[i] != 'R') {
            printf(
                "Found unknown flag option \"%c\" , only i, l, and R are "
                "supported\n",
                flags[i]);
            return false;
        }
        i++;
    }
    return true;
}

DIR *validatePath(const char *path) {
    DIR *pDir = opendir(path);
    if (pDir) {
        return pDir;
    } else {
        printf("Could not open the path \"%s\"\n", path);
        printf("path len: %d\n", strlen(path));
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        return NULL;
    }
}
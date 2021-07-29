#include "validators.h"

#include <stdio.h>
#include <string.h>

bool validateFlags(const char *flags) {
    if (!flags) {
        return true;
    }
    if (strlen(flags) > 3) {
        printf(
            "Maximum expected number of flags are 3 excluding \"-\", got %zd\n",
            strlen(flags));
        return false;
    }
    if (strncmp(flags, "-", 1) != 0 || strlen(flags) < 2) {
        printf("Expected \"-\" before flag(s), got \"%s\"\n", flags);
        return false;
    }
    return true;
}

DIR *validatePath(const char *path) {
    DIR *pDir = opendir(path);
    if (pDir) {
        return pDir;
    } else {
        printf("Could not open the path \"%s\"\n", path);
        return NULL;
    }
}
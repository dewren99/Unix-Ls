#include <dirent.h>
#include <stdbool.h>
#include <sys/types.h>

bool validateFlags(const char *flags);

DIR *validatePath(const char *path);

bool isFlagArg(char *arg);
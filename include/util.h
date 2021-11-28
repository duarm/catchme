#ifndef UTIL_H
#define UTIL_H

#include <wchar.h>

void die(const char* fmt, ...);
char *basename(char const *path);
char *repl_str(const char *str, const char *from, const char *to);
int msleep(long msec);

#endif // UTILS_H


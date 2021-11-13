#ifndef UTIL_H
#define UTIL_H

void die(const char* fmt, ...);
char *basename(char const *path);
char *str_replace(char *orig, char *rep, char *with);
int msleep(long msec);

#endif // UTILS_H


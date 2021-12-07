#ifndef UTIL_H
#define UTIL_H

void die(const char* fmt, ...);
char *basename(char const *path);
char *repl_str(const char *str, const char *from, const char *to);
int msleep(long msec);
int get_int(char *str, int *n);

#endif // UTILS_H


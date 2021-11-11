#ifndef SMPD_H
#define SMPD_H

#include <stdbool.h>

bool get_property_bool(const char *property, bool *result);
bool get_property_string(const char *property, char *result);
void get_property(const char* property, char* result);

#endif


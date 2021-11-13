#ifndef CATCHME_H
#define CATCHME_H

#include <stdbool.h>

bool get_metadata(const char *name, char *result);
bool get_property_bool(const char *property, bool *result);
bool get_property_string(const char *property, char *result);
bool get_property_double(const char *property, double *result);
bool get_property_int(const char *property, int *result);
void catchme_update(void);
void catchme_remove(int id);
void catchme_mute(void);
void catchme_toggle(void);
void catchme_play(void);
void catchme_seek(char *seek);
void catchme_idle(void);
void catchme_playlist(void);
void catchme_play_index(int index);
void catchme_status(void);
void catchme_current(void);
void catchme_pause(void);
void catchme_prev(void);
void catchme_next(void);
void catchme_volume(char *vol);
void catchme_shuffle(void);

#endif // CATCHME_H


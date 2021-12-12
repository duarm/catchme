#ifndef CATCHME_H
#define CATCHME_H

#include <stdbool.h>

enum {
	WRITE_BOTH,
	WRITE_NAME,
	WRITE_PATH,
};

bool get_metadata(const char *name, char *result, int result_size);
bool get_property_bool(const char *property, bool *result);
bool get_property_string(const char *property, char *result, int result_size);
bool get_property_double(const char *property, double *result);
bool get_property_int(const char *property, int *result);
char *get_artist_title();
void catchme_write(const int to);
void catchme_write_to(const char *path);
void catchme_add(const char *path);
void catchme_remove(const int id);
void catchme_mute(void);
void catchme_toggle(void);
void catchme_play(void);
void catchme_format(char *format);
void catchme_seek(const char *seek);
void catchme_idle(void);
void catchme_print_playlist(void);
void catchme_play_index(const int index);
void catchme_status(void);
void catchme_current(void);
void catchme_pause(void);
void catchme_prev(const int n);
void catchme_next(const int n);
void catchme_volume(const char *vol);
void catchme_shuffle(void);

#endif // CATCHME_H


#include "catchme.h"
#include "util.h"
#include "config.h"
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <json-c/json.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>

int fd = -1;
pid_t pid;
char cmdbuff[SOCKETBUF_SIZE];

static void usage(void)
{
	puts("usage: catchme [-vl [-h] COMMAND\n"
	     "COMMAND\n"
	     "	seek\n"
	     "	vol/volume [+/-]VOL\n"
	     "	play - Unpauses\n"
	     "	play-index ID - plays the music the the given ID\n"
	     "	pause - Pauses\n"
	     "	mute - Toggle mute\n"
	     "	repeat - Toggle repeat current music\n"
	     "	add PATH - Apends the music in the given path to the playlist\n"
	     "	remove ID - Removes the music at the given ID in the playlist\n"
	     "	toggle - Toggle pause\n"
	     "	status - Returns a status list of the current music ?REMOVE?\n"
	     "	current/curr - Returns the name of the current music\n"
	     "	prev - Plays previous music\n"
	     "	next - Plays next music\n"
	     "	clear - Clears the playlist\n"
	     "	idle - TODO\n"
	     "	playlist - Prints the whole playlist to stdout\n"
	     "	update - Updates the music_names_cache and music_paths_cache.");
	exit(EXIT_SUCCESS);
}

static void open_socket(void)
{
	//socket already open
	if (fd != -1)
		return;

	int len;
	struct sockaddr_un remote;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, socket_path, 108);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(fd, (struct sockaddr *)&remote, len) == -1) {
		perror("connect");
		exit(EXIT_FAILURE);
	}
}

static int send_to_socket(char *msg)
{
	int t;
	while (true) {
		if (send(fd, msg, strlen(cmdbuff), 0) == -1) {
			perror("send");
			exit(1);
		}
		if ((t = recv(fd, msg, SOCKETBUF_SIZE, 0)) > 0) {
			msg[t] = '\0';
			return true;
		} else {
			if (t < 0)
				perror("recv");
			else
				printf("Server closed connection\n");
			exit(EXIT_FAILURE);
		}
	}
}

static void get_filename(int index, char* result)
{

}

bool get_metadata(const char *name, char *result)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, METADATA_MSG, name);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *error = json_object_get_string(
			json_object_object_get(res, "error"));
		if (!strcmp(error, "success")) {
			const char *str = json_object_get_string(
				json_object_object_get(res, "data"));
			strncpy(result, str, DATABUF_SIZE);
			result[strlen(str)] = '\0';
			return true;
		}
		json_object_put(res);
	}
	return false;
}

bool get_property_int(const char *property, int *result)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PROPERTY_MSG, property);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		*result = json_object_get_int(
			json_object_object_get(res, "data"));
		json_object_put(res);
		return true;
	}
	return false;
}

bool get_property_string(const char *property, char *result)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PROPERTY_STRING_MSG, property);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *str = json_object_get_string(
			json_object_object_get(res, "data"));
		strncpy(result, str, strlen(str));
		json_object_put(res);
		return true;
	}
	return false;
}

bool get_property_double(const char *property, double *result)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PROPERTY_MSG, property);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		*result = json_object_get_double(
			json_object_object_get(res, "data"));
		json_object_put(res);
		return true;
	}
	return false;
}

bool get_property_bool(const char *property, bool *result)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PROPERTY_MSG, property);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		json_bool prop = json_object_get_boolean(
			json_object_object_get(res, "data"));
		*result = prop;
		json_object_put(res);
		return true;
	}
	return false;
}

void catchme_playlist_clear(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_CLEAR);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		catchme_update();
		json_object_put(res);
	}
}

void catchme_add(char *path)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_APPEND, path);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		catchme_update();
		json_object_put(res);
	}
}

void catchme_play_playlist(char *path)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_LOAD, path);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		msleep(500);
		catchme_update();
		json_object_put(res);
	}
}

void catchme_remove(int id)
{
	int current;
	get_property_int("playlist-pos", &current);

	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_REMOVE, id);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);

		// TODO: patch solution, find a better one
		// if we're removing the same track we're playing. we sleep for 0.5s
		// so mpv can perform an audio reconfig
		if (id == current)
			msleep(500);
		catchme_update();
	}
}

void catchme_repeat(void)
{
	bool loop;
	get_property_bool("loop-file", &loop);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "loop-file",
		 loop ? "no" : "inf");
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_save_playlist(void)
{
	//todo
}

void catchme_mute(void)
{
	bool mute;
	get_property_bool("mute", &mute);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "mute",
		 mute ? "no" : "yes");
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_toggle(void)
{
	bool pause;
	get_property_bool("pause", &pause);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause",
		 pause ? "no" : "yes");
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_play(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause", "no");

	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_seek(char *seek)
{
	double time = 0.0;

	// + or - prefix for relative, raw value for absolute
	if (seek[0] == '+') {
		get_property_double("playback-time", &time);
		char timebuff[4];
		int len = strlen(seek) - 1;
		memcpy(timebuff, &seek[1], len);
		timebuff[len + 1] = '\0';
		time += atof(timebuff);
	} else if (seek[0] == '-') {
		get_property_double("playback-time", &time);
		char timebuff[4];
		int len = strlen(seek) - 1;
		memcpy(timebuff, &seek[1], len);
		timebuff[len + 1] = '\0';
		time -= atof(timebuff);
	} else if (seek[strlen(seek) - 1] == '%') {
		char timebuff[4];
		int len = strlen(seek);
		// len size goes up to the null terminator,
		// -1 to go to up to the last number
		memcpy(timebuff, &seek[0], len - 1);
		timebuff[len - 1] = '\0';
		time = atof(timebuff);

		if (time > 100)
			time = 100;
		else if (time < 0)
			time = 0;

		snprintf(cmdbuff, SOCKETBUF_SIZE, SEEK_PERCENTAGE_MSG, time);
		if (send_to_socket(cmdbuff)) {
			struct json_object *res = json_tokener_parse(cmdbuff);
			/* json_object_get_string(json_object_object_get(res, "error")); */
			json_object_put(res);
		}
		return;
	} else {
		char timebuff[4];
		int len = strlen(seek) - 1;
		memcpy(timebuff, &seek[1], len);
		timebuff[len + 1] = '\0';
		time = atof(timebuff);
	}

	snprintf(cmdbuff, SOCKETBUF_SIZE, SEEK_MSG, time);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_idle(void)
{
}

void catchme_playlist(void)
{
	int playlist_len;
	get_property_int("playlist-count", &playlist_len);

	// we open with 'w' to clear it, then with reopen with 'a+' to append
	FILE *fp = fopen(MUSIC_PATHS_CACHE, "w");
	FILE *fn = fopen(MUSIC_NAMES_CACHE, "w");
	if (fp == NULL)
		die("error opening " MUSIC_PATHS_CACHE);
	if (fn == NULL)
		die("error opening " MUSIC_NAMES_CACHE);

	freopen(MUSIC_PATHS_CACHE, "a+", fp);
	freopen(MUSIC_NAMES_CACHE, "a+", fn);

	for (int i = 0; i < playlist_len; i++) {
		snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PLAYLIST_FILENAME_MSG, i);
		if (send_to_socket(cmdbuff)) {
			struct json_object *res = json_tokener_parse(cmdbuff);
			const char *str = json_object_get_string(
				json_object_object_get(res, "data"));
			printf("%s\n", str);
			fprintf(fp, "%s\n", str);
			char *bname = basename(str);
			fprintf(fn, "%s\n", bname);
			json_object_put(res);
			free(bname);
		}
	}
	fclose(fp);
	fclose(fn);
}

void catchme_play_index(int index)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, index);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_update(void)
{
	int playlist_len;
	get_property_int("playlist-count", &playlist_len);

	// we open with 'w' to clear it, then with reopen with 'a+' to append
	FILE *fp = fopen(MUSIC_PATHS_CACHE, "w");
	FILE *fn = fopen(MUSIC_NAMES_CACHE, "w");
	if (fp == NULL)
		die("error opening " MUSIC_PATHS_CACHE);
	if (fn == NULL)
		die("error opening " MUSIC_NAMES_CACHE);

	freopen(MUSIC_PATHS_CACHE, "a+", fp);
	freopen(MUSIC_NAMES_CACHE, "a+", fn);

	for (int i = 0; i < playlist_len; i++) {
		snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PLAYLIST_FILENAME_MSG, i);
		if (send_to_socket(cmdbuff)) {
			struct json_object *res = json_tokener_parse(cmdbuff);
			const char *str = json_object_get_string(
				json_object_object_get(res, "data"));
			fprintf(fp, "%s\n", str);
			char *bname = basename(str);
			fprintf(fn, "%s\n", bname);
			json_object_put(res);
			free(bname);
		}
	}
	fclose(fp);
	fclose(fn);
}

void catchme_status(void)
{
	char artist[DATABUF_SIZE];
	char title[DATABUF_SIZE];

	if (!get_metadata("artist", artist))
		strncpy(artist, "N/A", DATABUF_SIZE);

	if (!get_metadata("title", title))
		strncpy(title, "N/A", DATABUF_SIZE);

	bool status;
	get_property_bool("pause", &status);

	int pos;
	get_property_int("playlist-pos", &pos);

	int playlist_len;
	get_property_int("playlist-count", &playlist_len);

	int percent_pos;
	get_property_int("percent-pos", &percent_pos);

	int vol;
	get_property_int("volume", &vol);

	// time/duration (percentage)
	printf("%s - %s\n"
	       "[%s] #%d/%d %.2f/%.2f (%d%%)\n"
	       "speed: %.2fx volume: %d%% muted: %d repeat: %d single: %d",
	       artist, title, status ? "paused" : "playing", pos, playlist_len,
	       0.0, 0.0, percent_pos, 0.0, vol, 0, 0, 0);
}

void print_format(void)
{
	//todo
}

void catchme_current(void)
{
	char artist[DATABUF_SIZE];
	char title[DATABUF_SIZE];

	if (get_metadata("artist", artist) && get_metadata("title", title)) {
		printf("%s - %s", artist, title);
	}
	else
	{
		get_property_string("filename", cmdbuff);
		printf("%s", cmdbuff);
	}

}

void catchme_pause(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause", "yes");
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_prev(void)
{
	int current;
	get_property_int("playlist-pos", &current);

	if (current == 0)
		return;

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, current - 1);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_next(void)
{
	int current;
	get_property_int("playlist-pos", &current);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, current + 1);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_volume(char *vol)
{
	int volume;

	// + or - prefix for relative, raw value for absolute
	if (vol[0] == '+') {
		get_property_int("volume", &volume);
		// max 999
		char volbuff[4];
		int len = strlen(vol) - 1;
		memcpy(volbuff, &vol[1], len);
		volbuff[len + 1] = '\0';
		volume += atoi(volbuff);
	} else if (vol[0] == '-') {
		get_property_int("volume", &volume);
		// max 999
		char volbuff[4];
		int len = strlen(vol) - 1;
		memcpy(volbuff, &vol[1], len);
		volbuff[len + 1] = '\0';
		volume -= atoi(volbuff);
	} else {
		// max 999
		char volbuff[4];
		int len = strlen(vol);
		memcpy(volbuff, &vol[0], len);
		volbuff[len] = '\0';
		volume = atoi(volbuff);
	}

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_VOLUME_MSG, volume);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_shuffle(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SHUFFLE_PLAYLIST_MSG);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		catchme_update();
		json_object_put(res);
	}
}

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		/* these options take no arguments */
		if (!strcmp(argv[i], "toggle")) {
			open_socket();
			catchme_toggle();
		} else if (!strcmp(argv[i], "next")) {
			open_socket();
			catchme_next();
		} else if (!strcmp(argv[i], "prev")) {
			open_socket();
			catchme_prev();
		} else if (!strcmp(argv[i], "seek")) {
			open_socket();
			catchme_seek(argv[++i]);
		} else if (!strcmp(argv[i], "vol") ||
			   !strcmp(argv[i], "volume")) {
			open_socket();
			catchme_volume(argv[++i]);
		} else if (!strcmp(argv[i], "curr") ||
			   !strcmp(argv[i], "current")) {
			open_socket();
			catchme_current();
		} else if (!strcmp(argv[i], "play")) {
			open_socket();
			catchme_play();
		} else if (!strcmp(argv[i], "pause")) {
			open_socket();
			catchme_pause();
		} else if (!strcmp(argv[i], "shuff") ||
			   !strcmp(argv[i], "shuffle")) {
			open_socket();
			catchme_shuffle();
		} else if (!strcmp(argv[i], "play-index")) {
			open_socket();
			catchme_play_index(atoi(argv[++i]));
		} else if (!strcmp(argv[i], "status")) {
			open_socket();
			catchme_status();
		} else if (!strcmp(argv[i], "mute")) {
			open_socket();
			catchme_mute();
		} else if (!strcmp(argv[i], "remove")) {
			open_socket();
			catchme_remove(atoi(argv[++i]));
		} else if (!strcmp(argv[i], "add")) {
			open_socket();
			catchme_add(argv[++i]);
		} else if (!strcmp(argv[i], "update")) {
			open_socket();
			catchme_update();
		} else if (!strcmp(argv[i], "clear")) {
			open_socket();
			catchme_playlist_clear();
		} else if (!strcmp(argv[i], "repeat")) {
			open_socket();
			catchme_repeat();
		} else if (!strcmp(argv[i], "playlist")) {
			open_socket();
			catchme_playlist();
		} else if (!strcmp(argv[i], "playlist-play")) {
			open_socket();
			catchme_play_playlist(argv[++i]);
		} else if (!strcmp(argv[i], "idle")) {
			/* open_socket(); */
			/* catchme_idle(); //todo */
		} else if (!strcmp(argv[i], "-h"))
			usage();
		else if (!strcmp(argv[i], "-s")) {
			strncpy(socket_path, argv[++i], MAX_PATH_SIZE);
			socket_path[strlen(socket_path)] = '\0';
		} else if (!strcmp(argv[i], "-v")) {
			puts("catchme " VERSION);
			exit(EXIT_SUCCESS);
		} else
			usage();
	}

	if (fd >= 0) {
		close(fd);
	}

	return EXIT_SUCCESS;
}


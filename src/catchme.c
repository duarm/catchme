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
char cmdbuff[SOCKETBUF_SIZE];
char *databuff;

static void usage(void)
{
	printf("usage: catchme [-s SOCKET_PATH] [-p PATHS_CACHE] [-n NAMES_CACHE] [-vl [-h] COMMAND\n"
	       "socket path: %s\n"
	       "COMMAND\n"
	       "	play - Unpauses\n"
	       "	pause - Pauses\n"
	       "	toggle - Toggle pause\n"
	       "	seek [+/-]TIME - Increments [+], decrements [-] or sets the absolute time of the current music\n"
	       "	vol/volume [+/-]VOL - Increments [+], decrements [-] or sets the absolute value of volume\n"
	       "	prev - Plays previous music\n"
	       "	next - Plays next music\n"
	       "	play-index POS - Plays the music at the given POS in the playlist.\n"
	       "	playlist - Prints the whole playlist to stdout\n"
	       "	playlist-play FILE/PATH - REPLACES the current playlist with the one from the given PATH or FILE\n"
	       "	mute - Toggle mute\n"
	       "	repeat - Toggle repeat current music\n"
	       "	format FORMAT - Returns the string formatted accordingly, with information from the currently playing music\n"
	       "	add PATH - Apends the music in the given path to the playlist\n"
	       "	remove ID - Removes the music at the given ID in the playlist\n"
	       "	status - Returns a status list of the current music ?REMOVE?\n"
	       "	current/curr - Returns the name of the current music\n"
	       "	clear - Clears the playlist\n"
	       "	idle - TODO\n"
	       "	update - Updates the music_names_cache and music_paths_cache.\n"
	       "FORMAT\n"
	       "  ;name;, ;title;, ;artist;, ;album;, ;album-artist;,\n"
	       "  ;genre;, ;playlist-count;, ;playlist-pos;, ;percent-pos;,\n"
	       "  ;status;, ;volume;, ;muted;\n"
	       "TODO\n"
	       "  ;path;, ;single;, ;time;, ;precise_time;, ;speed;, ;length;, ;remaining;, ;repeat;",
	       socket_path);
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
			exit(EXIT_FAILURE);
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
		strncpy(result, str, DATABUF_SIZE - 1);
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
	int current = 0;
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
	bool loop = false;
	get_property_bool("loop-file", &loop);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "loop-file",
		 loop ? "no" : "inf");
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_mute(void)
{
	bool mute = false;
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
	bool pause = false;
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
	// max length
	const int len = 5;

	// + or - prefix for relative, raw value for absolute
	if (seek[0] == '+') {
		get_property_double("playback-time", &time);
		char timebuff[5];
		strncpy(timebuff, &seek[1], len);
		time += atof(timebuff);
	} else if (seek[0] == '-') {
		get_property_double("playback-time", &time);
		char timebuff[5];
		strncpy(timebuff, &seek[1], len);
		time -= atof(timebuff);
	} else if (seek[strlen(seek) - 1] == '%') {
		char timebuff[5];
		strncpy(timebuff, &seek[0], len - 1);
		printf("%s\n", timebuff);
		time = atoi(timebuff);

		if (time >= 100)
			time = 99;
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
		char timebuff[5];
		strncpy(timebuff, &seek[1], len);
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

bool get_filepath(char *result, int index)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PLAYLIST_FILENAME_MSG, index);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *str = json_object_get_string(
			json_object_object_get(res, "data"));
		strncpy(result, str, DATABUF_SIZE - 1);
		json_object_put(res);
		return true;
	}
	return false;
}

void catchme_playlist(void)
{
	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);

	// we open with 'w' to clear it, then reopen with 'a+' to append
	FILE *fp = fopen(music_path_cache, "w");
	FILE *fn = fopen(music_names_cache, "w");
	if (fp == NULL)
		die("error opening %s", music_path_cache);
	if (fn == NULL) {
		fclose(fp);
		die("error opening %s", music_names_cache);
	}

	freopen(music_path_cache, "a+", fp);
	freopen(music_names_cache, "a+", fn);

	for (int i = 0; i < playlist_len; i++) {
		if (get_filepath(cmdbuff, i)) {
			printf("%s\n", cmdbuff);
			fprintf(fp, "%s\n", cmdbuff);
			char *bname = basename(cmdbuff);
			fprintf(fn, "%s\n", bname);
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
	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);

	// we open with 'w' to clear it, then reopen with 'a+' to append
	FILE *fp = fopen(music_path_cache, "w");
	FILE *fn = fopen(music_names_cache, "w");
	if (fp == NULL)
		die("error opening %s", music_path_cache);
	if (fn == NULL) {
		fclose(fp);
		die("error opening %s", music_names_cache);
	}

	freopen(music_path_cache, "a+", fp);
	freopen(music_names_cache, "a+", fn);

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

	bool status = 0;
	get_property_bool("pause", &status);

	int pos = 0;
	get_property_int("playlist-pos", &pos);

	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);

	int percent_pos = 0;
	get_property_int("percent-pos", &percent_pos);

	int vol = 0;
	get_property_int("volume", &vol);

	// time/duration (percentage)
	printf("%s - %s\n"
	       "[%s] #%d/%d %.2f/%.2f (%d%%)\n"
	       "speed: %.2fx volume: %d%% muted: %d repeat: %d single: %d",
	       artist, title, status ? "paused" : "playing", pos, playlist_len,
	       0.0, 0.0, percent_pos, 0.0, vol, 0, 0, 0);
}

// there's a lot of room for improvement
void catchme_format(char *format)
{
	char data[DATABUF_SIZE];
	if (!get_metadata("artist", data))
		strncpy(data, "N/A", DATABUF_SIZE);
	char *result = repl_str(format, ";artist;", data);

	if (!get_metadata("title", data))
		strncpy(data, "N/A", DATABUF_SIZE);
	result = repl_str(result, ";title;", data);

	if (!get_metadata("album", data))
		strncpy(data, "N/A", DATABUF_SIZE);
	result = repl_str(result, ";album;", data);

	if (!get_metadata("genre", data))
		strncpy(data, "N/A", DATABUF_SIZE);
	result = repl_str(result, ";genre;", data);

	if (!get_metadata("album_artist", data))
		strncpy(data, "N/A", DATABUF_SIZE);
	result = repl_str(result, ";album-artist;", data);

	/* if (!get_metadata("comment", data)) */
	/* 	strncpy(data, "N/A", DATABUF_SIZE); */
	/* result = repl_str(result, "-comment-", data); */

	bool status = false;
	get_property_bool("pause", &status);
	snprintf(data, DATABUF_SIZE, "%s", status ? "paused" : "playing");
	result = repl_str(result, ";status;", data);

	int pos = 0;
	get_property_int("playlist-pos", &pos);
	snprintf(data, DATABUF_SIZE, "%d", pos);
	result = repl_str(result, ";playlist-pos;", data);

	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);
	snprintf(data, DATABUF_SIZE, "%d", playlist_len);
	result = repl_str(result, ";playlist-count;", data);

	int percent_pos = 0;
	get_property_int("percent-pos", &percent_pos);
	snprintf(data, DATABUF_SIZE, "%d", percent_pos);
	result = repl_str(result, ";percent-pos;", data);

	int vol = 0;
	get_property_int("volume", &vol);
	snprintf(data, DATABUF_SIZE, "%d", vol);
	result = repl_str(result, ";volume;", data);

	bool mute = false;
	get_property_bool("mute", &mute);
	snprintf(data, DATABUF_SIZE, "%s", mute ? "muted" : "unmuted");
	result = repl_str(result, ";muted;", data);

	printf("%s\n", result);
	free(result);
}

void catchme_current(void)
{
	char artist[DATABUF_SIZE];
	char title[DATABUF_SIZE];

	if (get_metadata("artist", artist) && get_metadata("title", title)) {
		printf("%s - %s", artist, title);
	} else {
		get_property_string("filename", title);
		printf("%s", title);
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
	int current = 0;
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
	int current = 0;
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
	int volume = 0;
	const int len = 5;
	if (!get_property_int("volume", &volume))
		return;

	// + or - prefix for relative, raw value for absolute
	if (vol[0] == '+') {
		char volbuff[5];
		strncpy(volbuff, &vol[1], len);
		volume += atoi(volbuff);
	} else if (vol[0] == '-') {
		char volbuff[5];
		strncpy(volbuff, &vol[1], len);
		volume -= atoi(volbuff);
	} else {
		char volbuff[5];
		strncpy(volbuff, &vol[0], len);
		volume = atoi(volbuff);
	}

	if (volume > MAX_VOLUME)
		volume = MAX_VOLUME;

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
		} else if (!strcmp(argv[i], "format")) {
			open_socket();
			catchme_format(argv[++i]);
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
		} else if (!strcmp(argv[i], "-h")) {
			usage();
			exit(EXIT_SUCCESS);
		} else if (!strcmp(argv[i], "-s")) {
			strncpy(socket_path, argv[++i], 107);
			socket_path[strlen(socket_path)] = '\0';
		} else if (!strcmp(argv[i], "-n")) {
			strncpy(music_names_cache, argv[++i],
				MAX_PATH_SIZE - 1);
			socket_path[strlen(music_names_cache)] = '\0';
		} else if (!strcmp(argv[i], "-p")) {
			strncpy(music_path_cache, argv[++i], MAX_PATH_SIZE - 1);
			socket_path[strlen(music_path_cache)] = '\0';
		} else if (!strcmp(argv[i], "-v")) {
			puts("catchme " VERSION);
			exit(EXIT_SUCCESS);
		} else {
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (fd >= 0) {
		close(fd);
	}

	return EXIT_SUCCESS;
}


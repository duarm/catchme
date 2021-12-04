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
	printf("usage: catchme [-s SOCKET_PATH] [-p PATHS_CACHE] [-n NAMES_CACHE] [-vl [-h] COMMAND ...\n"
	       "socket path: %s\n"
	       "COMMAND\n"
	       "	play [POS] - Unpauses, if POS is specified, plays the music at the given POS in the playlist.\n"
	       "	pause - Pauses\n"
	       "	toggle/tog - Toggle pause\n"
	       "	seek [+/-]TIME[%%] - Increments (+), decrements (-), set relative (%%) or set the absolute time of the current music\n"
	       "	vol/volume [+/-]VOL - Increments [+], decrements [-] or sets the absolute volume\n"
	       "	next [N] - Play next music, if N is specified, jump to N songs ahead\n"
	       "	prev [N] - Play the previous song, if N is specified, jump to N songs behind\n"
	       "	playlist - Prints the whole playlist to stdout\n"
	       "	playlis-play FILE/PATH - REPLACES the current playlist with the one from the given PATH or FILE\n"
	       "	mute - Toggle mute\n"
	       "	repeat - Toggle repeat current music\n"
	       "	format FORMAT - Returns the string formatted accordingly, with information from the currently playing music\n"
	       "	add PATH - Apends the music in the given path to the playlist\n"
	       "	remove POS - Removes the music at the given POS in the playlist\n"
	       "	status - Returns a status list of the current music\n"
	       "	current/curr - Returns ';artist; - ;title;' of the current music.\n"
	       "	clear - Clears the playlist\n"
	       "	shuffle/shuf - Shuffles the playlist\n"
	       "	idle - TODO\n"
	       "	write [path/name] - Writes to music_names_cache, music_paths_cache or both if nothing passed.\n"
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

// receives an already formatted msg, returns the data field from the response json as the result.
bool get_property(const char *msg, char *result)
{
	printf("msg: %s\n", msg);
	snprintf(cmdbuff, SOCKETBUF_SIZE, "%s", msg);
	if (send_to_socket(cmdbuff)) {
		printf("res: %s\n", cmdbuff);
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *str = json_object_get_string(
			json_object_object_get(res, "data"));
		strncpy(result, str, DATABUF_SIZE - 1);
		json_object_put(res);
		return true;
	}
	return false;
}

// receives a property, set the data converted to int as the result.
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

// receives a property, returns the data field from the response json as the result.
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

// receives a property, set the data converted to double as the result.
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

// receives a property, set the data converted to bool as the result.
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
		catchme_write(WRITE_BOTH);
		json_object_put(res);
	}
}

void catchme_add(const char *path)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_APPEND, path);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		catchme_write(WRITE_BOTH);
		json_object_put(res);
	}
}

void catchme_playlist_play(const char *path)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_LOAD, path);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		msleep(500);
		catchme_write(WRITE_BOTH);
		json_object_put(res);
	}
}

void catchme_remove(const int id)
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
		catchme_write(WRITE_BOTH);
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

void catchme_volume(const char *vol)
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
		/* struct json_object *res = json_tokener_parse(cmdbuff); */
		/* json_object_get_string(json_object_object_get(res, "error")); */
		/* json_object_put(res); */
	}
}

void catchme_seek(const char *seek)
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
		/* struct json_object *res = json_tokener_parse(cmdbuff); */
		/* json_object_get_string(json_object_object_get(res, "error")); */
		/* json_object_put(res); */
	}
}

bool get_filepath(char *result, const int index)
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

void catchme_play_index(const int index)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, index);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_write(const int to)
{
	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);

	FILE *fp = NULL;
	FILE *fn = NULL;

	// we open with 'w' to clear it, then reopen with 'a+' to append
	if (to == WRITE_PATH || to == WRITE_BOTH) {
		fp = fopen(music_path_cache, "w");
		if (fp == NULL)
			die("error opening %s", music_path_cache);
		freopen(music_path_cache, "a+", fp);
	}

	if (to == WRITE_NAME || to == WRITE_BOTH) {
		fn = fopen(music_names_cache, "w");

		if (fn == NULL)
			die("error opening %s", music_names_cache);

		freopen(music_names_cache, "a+", fn);
	}

	for (int i = 0; i < playlist_len; i++) {
		if (get_filepath(cmdbuff, i)) {
			if (to == WRITE_BOTH || to == WRITE_PATH) {
				printf("path: %s\n", cmdbuff);
				fprintf(fp, "%s\n", cmdbuff);
			}
			if (to == WRITE_BOTH || to == WRITE_NAME) {
				char *name = basename(cmdbuff);
				printf("nam: %s\n", name);
				fprintf(fn, "%s\n", name);
				free(name);
			}
		}
	}

	if (fp != NULL)
		fclose(fp);
	if (fn != NULL)
		fclose(fn);
}

void catchme_write_to(const char *path)
{
	int to = WRITE_BOTH;
	if (path == NULL) {
		to = WRITE_BOTH;
	} else if (!strncmp(path, "name", 5)) {
		to = WRITE_NAME;
	} else if (!strncmp(path, "path", 5)) {
		to = WRITE_PATH;
	} else
		exit(EXIT_FAILURE);
	catchme_write(to);
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

// must free the result
char *get_artist_title()
{
	// if there's no artist or title, we return the filename instead
	if (!get_metadata("artist", cmdbuff)) {
		int current = 0;
		get_property_int("playlist-pos", &current);
		get_filepath(cmdbuff, current);
		return basename(cmdbuff);
	}
	char *result = repl_str(";artist; - ;title;", ";artist;", cmdbuff);

	if (!get_metadata("title", cmdbuff)) {
		int current = 0;
		get_property_int("playlist-pos", &current);
		get_filepath(cmdbuff, current);
		return basename(cmdbuff);
	}
	return repl_str(result, ";title;", cmdbuff);
}

void catchme_current(void)
{
	char *result = get_artist_title();
	printf("%s\n", result);
	free(result);
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

void catchme_prev(const int n)
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

void catchme_next(const int n)
{
	int current = 0;
	get_property_int("playlist-pos", &current);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, current + n);
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
		catchme_write(WRITE_BOTH);
		json_object_put(res);
	}
}

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "toggle", 6) ||
		    !strncmp(argv[i], "tog", 3)) {
			open_socket();
			catchme_toggle();
		} else if (!strncmp(argv[i], "next", 4)) {
			open_socket();
			i++;
			if (i == argc)
				catchme_next(1);
			else
				catchme_next(atoi(argv[i]));
		} else if (!strncmp(argv[i], "prev", 4)) {
			open_socket();
			i++;
			if (i == argc)
				catchme_prev(1);
			else
				catchme_prev(atoi(argv[i]));
		} else if (!strncmp(argv[i], "seek", 4)) {
			open_socket();
			catchme_seek(argv[++i]);
		} else if (!strncmp(argv[i], "vol", 3) ||
			   !strncmp(argv[i], "volume", 6)) {
			open_socket();
			catchme_volume(argv[++i]);
		} else if (!strncmp(argv[i], "curr", 4) ||
			   !strncmp(argv[i], "current", 7)) {
			open_socket();
			catchme_current();
		} else if (!strncmp(argv[i], "play", 4)) {
			open_socket();
			i++;
			if (i == argc)
				catchme_play();
			else
				catchme_play_index(atoi(argv[i]));
		} else if (!strncmp(argv[i], "pause", 5)) {
			open_socket();
			catchme_pause();
		} else if (!strncmp(argv[i], "format", 6)) {
			open_socket();
			i++;
			if (i == argc)
				return EXIT_FAILURE;
			catchme_format(argv[i]);
		} else if (!strncmp(argv[i], "shuf", 4) ||
			   !strncmp(argv[i], "shuffle", 7)) {
			open_socket();
			catchme_shuffle();
		} else if (!strncmp(argv[i], "status", 6)) {
			open_socket();
			catchme_status();
		} else if (!strncmp(argv[i], "mute", 4)) {
			open_socket();
			catchme_mute();
		} else if (!strcmp(argv[i], "remove")) {
			open_socket();
			i++;
			if (i == argc)
				return EXIT_FAILURE;
			catchme_remove(atoi(argv[++i]));
		} else if (!strncmp(argv[i], "add", 3)) {
			open_socket();
			catchme_add(argv[++i]);
		} else if (!strncmp(argv[i], "write", 5)) {
			open_socket();
			catchme_write_to(argv[++i]);
		} else if (!strncmp(argv[i], "clear", 5)) {
			open_socket();
			catchme_playlist_clear();
		} else if (!strncmp(argv[i], "repeat", 6)) {
			open_socket();
			catchme_repeat();
		} else if (!strncmp(argv[i], "playlist", 8)) {
			open_socket();
			catchme_playlist();
		} else if (!strncmp(argv[i], "playlist-play", 13)) {
			open_socket();
			i++;
			if (i == argc)
				return EXIT_FAILURE;
			catchme_playlist_play(argv[++i]);
		} else if (!strncmp(argv[i], "idle", 4)) {
			/* open_socket(); */
			/* catchme_idle(); //todo */
		} else if (!strncmp(argv[i], "-h", 2)) {
			usage();
			exit(EXIT_SUCCESS);
		} else if (!strncmp(argv[i], "-s", 2)) {
			i++;
			if (i == argc)
				return EXIT_FAILURE;
			strncpy(socket_path, argv[i], 107);
			socket_path[strlen(socket_path)] = '\0';
		} else if (!strncmp(argv[i], "-n", 2)) {
			i++;
			if (i == argc)
				return EXIT_FAILURE;
			strncpy(music_names_cache, argv[i], MAX_PATH_SIZE - 1);
			socket_path[strlen(music_names_cache)] = '\0';
		} else if (!strncmp(argv[i], "-p", 2)) {
			i++;
			if (i + 1 == argc)
				return EXIT_FAILURE;
			strncpy(music_path_cache, argv[i], MAX_PATH_SIZE - 1);
			socket_path[strlen(music_path_cache)] = '\0';
		} else if (!strncmp(argv[i], "-v", 2)) {
			puts("catchme " VERSION);
			exit(EXIT_SUCCESS);
		} else {
			printf("invalid command\n");
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (fd >= 0) {
		close(fd);
	}

	return EXIT_SUCCESS;
}

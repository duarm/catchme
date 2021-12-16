#include "catchme.h"
#include "util.h"
#include "config.h"
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int fd = -1;
// cmd buff is used to copy the messages to the socket
char cmdbuff[SOCKETBUF_SIZE];
// databuff is used to store general data
// databuff is never implicitly written to unlike cmdbuff, which
// is used internally by most functions without the necessity to
// specify it as a parameter.
char databuff[DATABUF_SIZE];

static void usage(void)
{
	printf("usage: catchme [-s SOCKET_PATH] [-p PATHS_CACHE] [-n NAMES_CACHE] [-vl [-h] COMMAND ...\n"
	       "socket path: %s\n"
	       "COMMAND\n"
	       "	play [POS] - Unpauses, if POS is specified, plays the music at the given POS in the playlist.\n"
	       "	pause - Pauses\n"
	       "	toggle/tog - Toggle pause\n"
	       "	seek [+/-]TIME[%%] - Increments [+], decrements [-], set relative (%%) or set the absolute time of the current music\n"
	       "	volume/vol [+/-]VOL - Increments [+], decrements [-] or sets the absolute volume\n"
	       "	next [N] - Play next music, if N is specified, jump to N songs ahead\n"
	       "	previous/prev [N] - Play the previous song, if N is specified, jump to N songs behind\n"
	       "	print-playlist - Prints the current playlist to stdout\n"
	       "	playlist FILE/PATH - REPLACES the current playlist with the one from the given PATH or FILE\n"
	       "	mute - Toggle mute\n"
	       "	repeat - Toggle repeat current music\n"
	       "	format \";FORMAT;\" - Returns the string formatted accordingly, with information from the currently playing music\n"
	       "	add PATH - Apends the music in the given path to the playlist\n"
	       "	remove/rem POS - Removes the music at the given POS in the playlist\n"
	       "	stat/status - Returns a status list of the current music\n"
	       "	current/curr - Returns ';artist; - ;title;' of the current music.\n"
	       "	clear - Clears the playlist\n"
	       "	shuffle/shuf - Shuffles the playlist\n"
	       "	idle - TODO\n"
	       "	write [path/name] - Writes to music_names_cache, music_paths_cache or both if nothing passed.\n"
	       "OBS\n"
	       "	partial commands are valid as long they're not ambiguous, e.g. shuf=shuffle, tog=toggle, vol=volume,\n"
	       "	play=play, playl=playlist, playlist-p=playlist-play\n"
	       "FORMAT\n"
	       "  name, title, artist, album, album-artist,\n"
	       "  genre, playlist-count, playlist-pos, percent-pos,\n"
	       "  status, volume, mute, path, speed, loop-file\n"
	       "TODO\n"
	       "  time, precise_time, length, remaining",
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

// send 'msg' to socket, writes the response to 'result'
static int send_to_socket(char *msg, char *result)
{
	int t;
	while (true) {
		if (send(fd, msg, strlen(cmdbuff), 0) == -1) {
			perror("send");
			exit(EXIT_FAILURE);
		}
		if ((t = recv(fd, result, DATABUF_SIZE - 1, 0)) > 0) {
			msg[t] = '\0';
			return true;
		} else {
			if (t < 0)
				perror("recv");
			else
				fprintf(stderr, "Server closed connection\n");
			exit(EXIT_FAILURE);
		}
	}
}

bool get_metadata(const char *name, char *result, int result_size)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, METADATA_MSG, name);
	if (send_to_socket(cmdbuff, cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *error = json_object_get_string(
			json_object_object_get(res, "error"));
		if (!strncmp(error, "success", 7)) {
			const char *str = json_object_get_string(
				json_object_object_get(res, "data"));
			strncpy(result, str, result_size - 1);
			result[strlen(str)] = '\0';
			return true;
		}
		json_object_put(res);
	}
	return false;
}

// receives an already formatted msg, writes to result the data field from the response json.
bool get_property(const char *msg, char *result, int result_size)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, "%s", msg);
	if (send_to_socket(cmdbuff, cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *str = json_object_get_string(
			json_object_object_get(res, "data"));
		strncpy(result, str, result_size - 1);
		json_object_put(res);
		return true;
	}
	return false;
}

// receives a property, writes to result the data converted to int.
bool get_property_int(const char *property, int *result)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PROPERTY_MSG, property);
	if (send_to_socket(cmdbuff, cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		*result = json_object_get_int(
			json_object_object_get(res, "data"));
		json_object_put(res);
		return true;
	}
	return false;
}

// receives a property, writes to result the data field from the response json.
bool get_property_string(const char *property, char *result, int result_size)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PROPERTY_STRING_MSG, property);
	if (send_to_socket(cmdbuff, cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *str = json_object_get_string(
			json_object_object_get(res, "data"));
		strncpy(result, str, result_size - 1);
		result[result_size - 1] = '\0';
		json_object_put(res);
		return true;
	}
	return false;
}

// receives a property, set the data converted to double as the result.
bool get_property_double(const char *property, double *result)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PROPERTY_MSG, property);
	if (send_to_socket(cmdbuff, cmdbuff)) {
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
	if (send_to_socket(cmdbuff, cmdbuff)) {
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
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_add(const char *path)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_APPEND, path);
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_playlist(const char *path)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_LOAD, path);
	send_to_socket(cmdbuff, cmdbuff);
	msleep(500);
}

void catchme_remove(const int id)
{
	int current = 0;
	get_property_int("playlist-pos", &current);

	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_REMOVE, id);
	send_to_socket(cmdbuff, cmdbuff);
	if (id == current)
		msleep(500);
}

void catchme_repeat(void)
{
	bool loop = false;
	get_property_bool("loop-file", &loop);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "loop-file",
		 loop ? "no" : "inf");
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_mute(void)
{
	bool mute = false;
	get_property_bool("mute", &mute);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "mute",
		 mute ? "no" : "yes");
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_toggle(void)
{
	bool pause = false;
	get_property_bool("pause", &pause);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause",
		 pause ? "no" : "yes");
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_play(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause", "no");
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_volume(const char *vol)
{
	int volume = 0;
	const int len = 5;
	if (!get_property_int("volume", &volume))
		return;

	// + or - prefix for relative, raw value for absolute
	if (vol[0] == '+') {
		char volbuff[6];
		strncpy(volbuff, &vol[1], len);
		volume += atoi(volbuff);
	} else if (vol[0] == '-') {
		char volbuff[6];
		strncpy(volbuff, &vol[1], len);
		volume -= atoi(volbuff);
	} else {
		char volbuff[6];
		strncpy(volbuff, &vol[0], len);
		volume = atoi(volbuff);
	}

	if (volume > MAX_VOLUME)
		volume = MAX_VOLUME;

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_VOLUME_MSG, volume);
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_seek(const char *seek)
{
	double time = 0.0;
	// max length
	const int len = 5;

	// + or - prefix for relative, raw value for absolute
	if (seek[0] == '+') {
		get_property_double("playback-time", &time);
		char timebuff[6];
		strncpy(timebuff, &seek[1], len);
		time += atof(timebuff);
	} else if (seek[0] == '-') {
		get_property_double("playback-time", &time);
		char timebuff[6];
		strncpy(timebuff, &seek[1], len);
		time -= atof(timebuff);
	} else if (seek[strlen(seek) - 1] == '%') {
		char timebuff[6];
		strncpy(timebuff, &seek[0], len - 1);
		time = atoi(timebuff);

		if (time >= 100)
			time = 99;
		else if (time < 0)
			time = 0;

		snprintf(cmdbuff, SOCKETBUF_SIZE, SEEK_PERCENTAGE_MSG, time);
		if (send_to_socket(cmdbuff, cmdbuff)) {
			struct json_object *res = json_tokener_parse(cmdbuff);
			/* json_object_get_string(json_object_object_get(res, "error")); */
			json_object_put(res);
		}
		return;
	} else {
		char timebuff[6];
		strncpy(timebuff, &seek[1], len);
		time = atof(timebuff);
	}

	snprintf(cmdbuff, SOCKETBUF_SIZE, SEEK_MSG, time);
	send_to_socket(cmdbuff, cmdbuff);
}

// result to store the filepath,
// playlist index of the music to retrieve the filepath
bool get_filepath(char *result, const int index)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PLAYLIST_FILENAME_MSG, index);
	if (send_to_socket(cmdbuff, result)) {
		struct json_object *res = json_tokener_parse(result);
		const char *str = json_object_get_string(
			json_object_object_get(res, "data"));
		strncpy(result, str, DATABUF_SIZE - 1);
		json_object_put(res);
		return true;
	}
	return false;
}

void catchme_print_playlist(void)
{
	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);

	for (int i = 0; i < playlist_len; i++) {
		if (get_filepath(databuff, i)) {
			printf("%s\n", databuff);
		}
	}
}

void catchme_play_index(const int index)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, index);
	send_to_socket(cmdbuff, cmdbuff);
	msleep(100);
}

void catchme_write(const int to)
{
	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);

	FILE *fp = NULL;
	FILE *fn = NULL;

	// we open with 'w' to clear it, then reopen with 'a+' to append
	if (to == WRITE_PATH || to == WRITE_BOTH) {
		fp = fopen(music_path_cache, "wa+");
		if (fp == NULL)
			die("error opening %s", music_path_cache);
	}

	if (to == WRITE_NAME || to == WRITE_BOTH) {
		fn = fopen(music_names_cache, "wa+");
		if (fn == NULL)
			die("error opening %s", music_names_cache);
	}

	for (int i = 0; i < playlist_len; i++) {
		if (get_filepath(databuff, i)) {
			if (to == WRITE_PATH || to == WRITE_BOTH) {
				fprintf(fp, "%s\n", databuff);
			}
			if (to == WRITE_NAME || to == WRITE_BOTH) {
				char *name = basename(databuff);
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
	} else {
		fprintf(stderr, "invalid write parameter, possible values: path, name, [EMPTY].\n");
		exit(EXIT_FAILURE);
	}
	catchme_write(to);
}

void catchme_status(void)
{
	char artist[DATABUF_SIZE];
	char title[DATABUF_SIZE];

	if (!get_metadata("artist", artist, DATABUF_SIZE))
		strncpy(artist, "N/A", DATABUF_SIZE);

	if (!get_metadata("title", title, DATABUF_SIZE))
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

	double speed = 0.0;
	get_property_double("speed", &speed);

	get_property_string("loop-file", databuff, DATABUF_SIZE);

	bool mute = false;
	get_property_bool("mute", &mute);

	// time/duration (percentage)
	printf("%s - %s\n"
	       "[%s] #%d/%d %.2f/%.2f (%d%%)\n"
	       "speed: %.2fx volume: %d%% muted: %d loop: %s\n",
	       artist, title, status ? "paused" : "playing", pos, playlist_len,
	       0.0, 0.0, percent_pos, speed, vol, mute, databuff);
}

// there's a lot of room for improvement
void catchme_format(char *format)
{
	if (!get_metadata("artist", databuff, DATABUF_SIZE))
		strncpy(databuff, "N/A", DATABUF_SIZE);
	char *result = repl_str(format, ";artist;", databuff);

	if (!get_metadata("title", databuff, DATABUF_SIZE))
		strncpy(databuff, "N/A", DATABUF_SIZE);
	result = repl_str(result, ";title;", databuff);

	if (!get_metadata("album", databuff, DATABUF_SIZE))
		strncpy(databuff, "N/A", DATABUF_SIZE);
	result = repl_str(result, ";album;", databuff);

	if (!get_metadata("genre", databuff, DATABUF_SIZE))
		strncpy(databuff, "N/A", DATABUF_SIZE);
	result = repl_str(result, ";genre;", databuff);

	if (!get_metadata("album_artist", databuff, DATABUF_SIZE))
		strncpy(databuff, "N/A", DATABUF_SIZE);
	result = repl_str(result, ";album-artist;", databuff);

	/* if (!get_metadata("comment", data)) */
	/* 	strncpy(data, "N/A", DATABUF_SIZE); */
	/* result = repl_str(result, "-comment-", data); */

	bool status = false;
	get_property_bool("pause", &status);
	snprintf(databuff, DATABUF_SIZE, "%s", status ? "paused" : "playing");
	result = repl_str(result, ";status;", databuff);

	int pos = 0;
	get_property_int("playlist-pos", &pos);
	snprintf(databuff, DATABUF_SIZE, "%d", pos);
	result = repl_str(result, ";playlist-pos;", databuff);

	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);
	snprintf(databuff, DATABUF_SIZE, "%d", playlist_len);
	result = repl_str(result, ";playlist-count;", databuff);

	int percent_pos = 0;
	get_property_int("percent-pos", &percent_pos);
	snprintf(databuff, DATABUF_SIZE, "%d", percent_pos);
	result = repl_str(result, ";percent-pos;", databuff);

	int vol = 0;
	get_property_int("volume", &vol);
	snprintf(databuff, DATABUF_SIZE, "%d", vol);
	result = repl_str(result, ";volume;", databuff);

	bool mute = false;
	get_property_bool("mute", &mute);
	snprintf(databuff, DATABUF_SIZE, "%d", mute);
	result = repl_str(result, ";mute;", databuff);

	bool loop = false;
	get_property_bool("loop-file", &loop);
	snprintf(databuff, DATABUF_SIZE, "%d", loop);
	result = repl_str(result, ";loop-file;", databuff);

	double speed = 0.0;
	get_property_double("speed", &speed);
	snprintf(databuff, DATABUF_SIZE, "%.2f", speed);
	result = repl_str(result, ";speed;", databuff);

	get_filepath(databuff, pos);
	result = repl_str(result, ";path;", databuff);

	printf("%s\n", result);
	free(result);
}

// must free the result
char *get_artist_title()
{
	// if there's no artist or title, we return the filename instead
	if (!get_metadata("artist", databuff, DATABUF_SIZE)) {
		int current = 0;
		get_property_int("playlist-pos", &current);
		get_filepath(databuff, current);
		return basename(databuff);
	}
	char *result = repl_str(";artist; - ;title;", ";artist;", databuff);

	if (!get_metadata("title", databuff, DATABUF_SIZE)) {
		int current = 0;
		get_property_int("playlist-pos", &current);
		get_filepath(databuff, current);
		return basename(databuff);
	}
	return repl_str(result, ";title;", databuff);
}

void catchme_current(void)
{
	char *result = get_artist_title();
	printf("%s", result);
	free(result);
}

void catchme_pause(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause", "yes");
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_prev(const int n)
{
	int current = 0;
	get_property_int("playlist-pos", &current);

	if (current == 0)
		return;

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, current - 1);
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_next(const int n)
{
	int current = 0;
	get_property_int("playlist-pos", &current);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, current + n);
	send_to_socket(cmdbuff, cmdbuff);
}

void catchme_shuffle(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SHUFFLE_PLAYLIST_MSG);
	send_to_socket(cmdbuff, databuff);
	msleep(500);
}

int main(int argc, char *argv[])
{
	int n;
	for (int i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "tog", 3)) { // tog/toggle
			open_socket();
			catchme_toggle();
		} else if (!strncmp(argv[i], "next", 4)) {
			i++;
			if (i == argc) {
				// if no argument, next music
				open_socket();
				catchme_next(1);
			} else if (get_int(argv[i], &n)) {
				// otherwise, jump to 'current + n'
				open_socket();
				catchme_next(n);
			} else {
				fprintf(stderr, "invalid integer for next\n");
				return EXIT_FAILURE;
			}
		} else if (!strncmp(argv[i], "prev", 4)) { // prev/previous
			i++;
			if (i == argc) {
				// if no argument, previous music
				open_socket();
				catchme_prev(1);
			} else if (get_int(argv[i], &n)) {
				// otherwise, jump to 'current - n'
				open_socket();
				catchme_prev(n);
			} else {
				fprintf(stderr, "invalid integer for prev\n");
				return EXIT_FAILURE;
			}
		} else if (!strncmp(argv[i], "seek", 4)) {
			i++;
			if (i == argc) {
				fprintf(stderr, "seek requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			}
			open_socket();
			catchme_seek(argv[i]);
		} else if (!strncmp(argv[i], "vol", 3)) { // vol/volume
			i++;
			if (i == argc) {
				fprintf(stderr, "vol requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			}
			open_socket();
			catchme_volume(argv[i]);
		} else if (!strncmp(argv[i], "curr", 4)) { // curr/current
			open_socket();
			catchme_current();
		} else if (!strncmp(argv[i], "play", 5)) {
			i++;
			if (i == argc) {
				// if no argument, unpauses
				open_socket();
				catchme_play();
			} else if (get_int(argv[i], &n)) {
				// otherwise, play the given valid integer
				open_socket();
				catchme_play_index(n);
			} else {
				fprintf(stderr, "invalid integer for play\n");
				return EXIT_FAILURE;
			}
		} else if (!strncmp(argv[i], "pause", 5)) {
			open_socket();
			catchme_pause();
		} else if (!strncmp(argv[i], "format", 6)) {
			i++;
			if (i == argc) {
				fprintf(stderr, "format requires 1 string parameter, none given.\n");
				return EXIT_FAILURE;
			}
			open_socket();
			catchme_format(argv[i]);
		} else if (!strncmp(argv[i], "shuf", 4)) { // shuf/shuffle
			open_socket();
			catchme_shuffle();
		} else if (!strncmp(argv[i], "stat", 4)) { // stat/status
			open_socket();
			catchme_status();
		} else if (!strncmp(argv[i], "mute", 4)) {
			open_socket();
			catchme_mute();
		} else if (!strncmp(argv[i], "rem", 3)) { // rem/remove
			i++;
			if (i == argc) {
				fprintf(stderr, "remove requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			} else if (!get_int(argv[i], &n)) {
				fprintf(stderr, "invalid integer for remove\n");
				return EXIT_FAILURE;
			}
			open_socket();
			catchme_remove(n);
		} else if (!strncmp(argv[i], "add", 3)) {
			i++;
			if (i == argc) {
				fprintf(stderr, "no path given.\n");
				return EXIT_FAILURE;
			}
			open_socket();
			catchme_add(argv[i]);
		} else if (!strncmp(argv[i], "write", 5)) {
			open_socket();
			catchme_write_to(argv[++i]);
		} else if (!strncmp(argv[i], "clear", 5)) {
			open_socket();
			catchme_playlist_clear();
		} else if (!strncmp(argv[i], "repeat", 6)) {
			open_socket();
			catchme_repeat();
		} else if (!strncmp(argv[i], "playlist", 5)) {
			i++;
			if (i == argc)
				return EXIT_FAILURE;
			open_socket();
			catchme_playlist(argv[i]);
		} else if (!strncmp(argv[i], "print-playlist", 14)) {
			open_socket();
			catchme_print_playlist();
		} else if (!strncmp(argv[i], "-h", 2)) {
			usage();
			return EXIT_SUCCESS;
		} else if (!strncmp(argv[i], "-s", 2)) {
			i++;
			if (i == argc) {
				fprintf(stderr, "-s requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			}
			strncpy(socket_path, argv[i], 107);
			socket_path[strlen(socket_path)] = '\0';
		} else if (!strncmp(argv[i], "-n", 2)) {
			i++;
			if (i == argc){
				fprintf(stderr, "-n requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			}
			strncpy(music_names_cache, argv[i], MAX_PATH_SIZE - 1);
			socket_path[strlen(music_names_cache)] = '\0';
		} else if (!strncmp(argv[i], "-p", 2)) {
			i++;
			if (i == argc){
				fprintf(stderr, "-p requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			}
			strncpy(music_path_cache, argv[i], MAX_PATH_SIZE - 1);
			socket_path[strlen(music_path_cache)] = '\0';
		} else if (!strncmp(argv[i], "-v", 2)) {
			puts("catchme " VERSION);
			return EXIT_SUCCESS;
		} else {
			usage();
			fprintf(stderr, "invalid command\n");
			return EXIT_FAILURE;
		}
	}

	if (fd >= 0) {
		close(fd);
	}

	return EXIT_SUCCESS;
}

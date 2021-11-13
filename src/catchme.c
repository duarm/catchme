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
char databuff[DATABUF_SIZE];

static void usage(void)
{
	puts("usage: catchme [-vl [-h] COMMAND\n"
	     "COMMAND\n"
	     "	seek\n"
	     "	vol/volume\n"
	     "	play\n"
	     "	play-id\n"
	     "	pause\n"
	     "	mute\n"
	     "	add\n"
	     "	remove\n"
	     "	toggle\n"
	     "	status\n"
	     "	current/curr\n"
	     "	prev\n"
	     "	next\n"
	     "	idle\n"
	     "	playlist\n"
	     "	update\n"
	     "\n");
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
	strcpy(remote.sun_path, SOCKET_FILE);
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

bool get_metadata(const char *name, char *result)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, METADATA_MSG, name);
	if (send_to_socket(cmdbuff)) {
		/* printf("res: %s\n", cmdbuff); */
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *str = json_object_get_string(
			json_object_object_get(res, "data"));
		strncpy(result, str, strlen(str));
		result[strlen(str)] = '\0';
		json_object_put(res);
		return true;
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
	snprintf(cmdbuff, SOCKETBUF_SIZE, GET_PROPERTY_MSG, property);
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

void catchme_move_music(void)
{
}

void catchme_add(void)
{
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
	//todo
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

	snprintf(cmdbuff, SOCKETBUF_SIZE, TOGGLE_MSG, pause ? "no" : "yes");
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_play(void)
{
	open_socket();
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAY_MSG);

	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_seek(char *seek)
{
	double time = 0.0;

	open_socket();

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

void catchme_play_id(int id)
{
	open_socket();

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, id);
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
	get_metadata("artist", databuff);
	char artist[strlen(databuff)];
	strcpy(artist, databuff);

	get_metadata("title", databuff);
	char title[strlen(databuff)];
	strcpy(title, databuff);

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
	open_socket();

	get_metadata("artist", databuff);
	char artist[strlen(databuff)];
	strcpy(artist, databuff);

	get_metadata("title", databuff);
	char title[strlen(databuff)];
	strcpy(title, databuff);

	printf("%s - %s", artist, title);
}

void catchme_pause(void)
{
	open_socket();

	snprintf(cmdbuff, SOCKETBUF_SIZE, PAUSE_MSG);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void catchme_prev(void)
{
	open_socket();

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
	open_socket();

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

	open_socket();

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
		if (!strcmp(argv[i], "-v")) { /* prints version information */
			puts("catchme " VERSION);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(argv[i], "-h"))
			usage();
		else if (!strcmp(argv[i], "play"))
			catchme_play();
		else if (!strcmp(argv[i], "pause"))
			catchme_pause();
		else if (!strcmp(argv[i], "toggle")) {
			open_socket();
			catchme_toggle();
		} else if (!strcmp(argv[i], "next"))
			catchme_next();
		else if (!strcmp(argv[i], "prev"))
			catchme_prev();
		else if (!strcmp(argv[i], "idle"))
			catchme_idle(); //todo
		else if (!strcmp(argv[i], "shuff") ||
			 !strcmp(argv[i], "shuffle")) {
			open_socket();
			catchme_shuffle();
		} else if (!strcmp(argv[i], "curr") ||
			   !strcmp(argv[i], "current"))
			catchme_current();
		else if (!strcmp(argv[i], "status")) {
			open_socket();
			catchme_status();
		} else if (!strcmp(argv[i], "playlist")) {
			open_socket();
			catchme_playlist();
		} else if (!strcmp(argv[i], "mute")) {
			open_socket();
			catchme_mute();
		} else if (!strcmp(argv[i], "add")) {
			open_socket();
			catchme_add();
		} else if (!strcmp(argv[i], "update")) {
			open_socket();
			catchme_update();
		}
		// one arg commands
		else if (!strcmp(argv[i], "remove")) {
			open_socket();
			catchme_remove(atoi(argv[++i]));
		} else if (!strcmp(argv[i], "seek"))
			catchme_seek(argv[++i]);
		else if (!strcmp(argv[i], "vol") ||
			 !strcmp(argv[i], "volume")) {
			catchme_volume(argv[++i]);
		} else if (!strcmp(argv[i], "play-id")) {
			catchme_play_id(atoi(argv[++i]));
		} else
			usage();
	}

	if (fd >= 0) {
		close(fd);
	}

	return EXIT_SUCCESS;
}


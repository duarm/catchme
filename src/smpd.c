#include "smpd.h"
#include "config.h"
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <json-c/json.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>

int fd;
char cmdbuff[SOCKETBUF_SIZE];
char databuff[DATABUF_SIZE];

static void usage(void)
{
	fputs("usage: smpd [-bfiv] [-l lines] [-p prompt] [-fn font] [-m monitor]\n"
	      "             [-nb color] [-nf color] [-sb color] [-sf color] [-w "
	      "windowid]\n",
	      stderr);
	exit(EXIT_SUCCESS);
}

static void open_socket(void)
{
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

void daemonize(void)
{
	system(SERVER_COMMAND);
}

bool get_metadata(const char *name, char *result)
{
	sprintf(cmdbuff, METADATA_MSG, name);
	if (send_to_socket(cmdbuff)) {
		/* printf("res: %s\n", cmdbuff); */
		struct json_object *res = json_tokener_parse(cmdbuff);
		const char *str = json_object_get_string(
			json_object_object_get(res, "data"));
		strncpy(result, str, strlen(str));
		json_object_put(res);
		return true;
	}
	return false;
}

bool get_property_int(const char *property, int *result)
{
	sprintf(cmdbuff, GET_PROPERTY_MSG, property);
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
	sprintf(cmdbuff, GET_PROPERTY_MSG, property);
	printf("sending %s\n", cmdbuff);
	if (send_to_socket(cmdbuff)) {
		printf("res: %s\n", cmdbuff);
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
	sprintf(cmdbuff, GET_PROPERTY_MSG, property);
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
	sprintf(cmdbuff, GET_PROPERTY_MSG, property);
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

void toggle(void)
{
	open_socket();
	bool pause;
	get_property_bool("pause", &pause);

	sprintf(cmdbuff, TOGGLE_MSG, pause ? "no" : "yes");
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void play(void)
{
	open_socket();
	sprintf(cmdbuff, PLAY_MSG);

	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void smpd_seek(char *seek)
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

		sprintf(cmdbuff, SEEK_PERCENTAGE_MSG, time);
		if (send_to_socket(cmdbuff)) {
			printf("res: %s\n", cmdbuff);
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

	sprintf(cmdbuff, SEEK_MSG, time);
	if (send_to_socket(cmdbuff)) {
		printf("res: %s\n", cmdbuff);
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void smpd_playlist(void)
{
}

void smpd_update(void)
{
}

void smpd_status(void)
{
	open_socket();

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

void smpd_current(void)
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

void smpd_pause(void)
{
	open_socket();

	sprintf(cmdbuff, PAUSE_MSG);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void prev(void)
{
	open_socket();

	int current;
	get_property_int("playlist-pos", &current);

	if (current == 0)
		return;

	sprintf(cmdbuff, PREV_PLAYLIST_MSG, current - 1);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void next(void)
{
	open_socket();

	int current;
	get_property_int("playlist-pos", &current);

	sprintf(cmdbuff, NEXT_PLAYLIST_MSG, current + 1);
	if (send_to_socket(cmdbuff)) {
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void volume(char *vol)
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

	sprintf(cmdbuff, VOLUME_MSG, volume);
	if (send_to_socket(cmdbuff)) {
		printf("res: %s\n", cmdbuff);
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

void shuffle(void)
{
	open_socket();

	sprintf(cmdbuff, SHUFFLE_PLAYLIST_MSG);
	if (send_to_socket(cmdbuff)) {
		printf("res: %s\n", cmdbuff);
		struct json_object *res = json_tokener_parse(cmdbuff);
		/* json_object_get_string(json_object_object_get(res, "error")); */
		json_object_put(res);
	}
}

int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		/* these options take no arguments */
		if (!strcmp(argv[i], "-v")) { /* prints version information */
			puts("smpd " VERSION);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(argv[i], "-h"))
			usage();
		else if (!strcmp(argv[i], "-d"))
			daemonize();
		else if (!strcmp(argv[i], "play"))
			play();
		else if (!strcmp(argv[i], "pause"))
			pause();
		else if (!strcmp(argv[i], "toggle"))
			toggle();
		else if (!strcmp(argv[i], "next"))
			next();
		else if (!strcmp(argv[i], "prev"))
			prev();
		else if (!strcmp(argv[i], "idle")) {
			//todo
		} else if (!strcmp(argv[i], "shuff") ||
			   !strcmp(argv[i], "shuffle"))
			shuffle();
		else if (!strcmp(argv[i], "curr") ||
			 !strcmp(argv[i], "current")) {
			smpd_current();
		} else if (!strcmp(argv[i], "status")) {
			smpd_status();
		} else if (!strcmp(argv[i], "playlist")) {
			smpd_playlist();
		} else if (!strcmp(argv[i], "update")) {
			smpd_update();
		} else if (!strcmp(argv[i], "seek"))
			smpd_seek(argv[++i]);
		// one arg commands
		else if (!strcmp(argv[i], "vol") ||
			 !strcmp(argv[i], "volume")) {
			volume(argv[++i]);
		} else
			usage();
	}

	if (fd >= 0) {
		close(fd);
	}

	unlink(SOCKET_FILE);
	return EXIT_SUCCESS;
}


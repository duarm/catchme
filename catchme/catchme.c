#include <catchme/util.h>
#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <json-c/json.h>

#define SCRIPT_CUSTOM_COMMAND "{ \"command\": [\"script-message\", \"%s\"] }\n"
#define SCRIPT_CUSTOM_COMMAND_PARAMS                                           \
	"{ \"command\": [\"script-message-to\", \"%s\", %s] }\n"
#define SCRIPT_CUSTOM_COMMAND_CATCHME                                          \
	"{ \"command\": [\"script-message-to\", \"mpv_catchme\", \"%s\"] }\n"
#define SCRIPT_CUSTOM_COMMAND_CATCHME_PARAMS                                   \
	"{ \"command\": [\"script-message-to\", \"mpv_catchme\", \"%s\", %s] }\n"

#define PLAYLIST_NEXT "{\"command\":[\"playlist-next\"]}\n"
#define PLAYLIST_PREV "{\"command\":[\"playlist-prev\"]}\n"
#define SET_PLAYING_MSG                                                        \
	"{ \"command\": [\"set_property\", \"playlist-pos\", %d ] }\n" // index
#define LOOP_FILE_MSG                                                          \
	"{ \"command\": [\"set_property_string\", \"loop-file\", \"%s\" ] }\n" // loop (inf, no)
#define SET_PROPERTY_MSG                                                       \
	"{ \"command\": [\"set_property\", \"%s\", \"%s\"] }\n" // property, parameter
#define GET_PROPERTY_MSG                                                       \
	"{ \"command\": [\"get_property\", \"%s\"] }\n" // property
#define PLAYLIST_LOAD "{ \"command\": [\"loadlist\", \"%s\" ] }\n" // file
#define PLAYLIST_APPEND                                                        \
	"{ \"command\": [\"loadfile\", \"%s\", \"append\" ] }\n" // file
#define PLAYLIST_CLEAR "{ \"command\": [\"playlist-clear\"] }\n"
#define PLAYLIST_REMOVE                                                        \
	"{ \"command\": [\"playlist-remove\", \"%d\"] }\n" // index
#define GET_PROPERTY_STRING_MSG                                                \
	"{ \"command\": [\"get_property_string\", \"%s\"] }\n" // property
#define GET_PLAYLIST_FILENAME_MSG                                              \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/filename\"] }\n" // index
#define GET_PLAYLIST_ARTIST_MSG                                                \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/artist\"] }\n" // index
#define GET_PLAYLIST_TITLE_MSG                                                 \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/title\"] }\n" // index
#define SHUFFLE_PLAYLIST_MSG "{ \"command\": [\"playlist-shuffle\" ] }\n"
#define SET_VOLUME_MSG                                                         \
	"{ \"command\": [\"set_property\", \"volume\", %d ] }\n" // volume
#define SEEK_PERCENTAGE_MSG                                                    \
	"{ \"command\": [\"set_property\", \"percent-pos\", %f ] }\n" // percentage
#define SEEK_MSG                                                               \
	"{ \"command\": [\"set_property\", \"playback-time\", %f ] }\n" // time
#define METADATA_MSG                                                           \
	"{ \"command\": [\"get_property\", \"metadata/by-key/%s\" ] }\n" // metadata

static bool get_metadata(const char *name, char *result, int result_size);
static bool get_property_bool(const char *property, bool *result);
static bool get_property_string(const char *property, char *result,
				int result_size);
static bool get_property_double(const char *property, double *result);
static bool get_property_int(const char *property, int *result);
static char *get_artist_title();
static int handle_custom_function(bool catchme, int argc, char *argv[],
				  int index);

static void cm_add_many(char *path[], int start, int count);
static void cm_add(const char *path);
static void cm_remove(const int id);
static void cm_mute(void);
static void cm_toggle(void);
static void cm_play(void);
static void cm_format(char *format);
static void cm_seek(const char *seek);
// static void cm_idle(void); TODO
static void cm_print_playlist(void);
static void cm_play_index(const int index);
static void cm_status(void);
static void cm_current(void);
static void cm_pause(void);
static void cm_next(const int n);
static void cm_volume(const char *vol);
static void cm_shuffle(void);

int fd = -1;
// `cmdbuff` is used to copy messages to send to the socket
char cmdbuff[SOCKETBUF_SIZE];
// databuff is used to store general data
char databuff[DATABUF_SIZE];

static void usage(void)
{
	printf("usage: catchme [-s SOCKET_PATH] [-h] COMMAND ...\n"
	       "version: " VERSION "\n"
	       "socket path: %s\n"
	       "COMMAND\n"
	       "	play [POS] - Unpauses. if [POS], plays the music at the given POS in the playlist\n"
	       "	pause - Pauses the currently playing song\n"
	       "	tog - Toggle pause\n"
	       "	seek [+/-]TIME[%%] - Set time to TIME. If [+/-], increment/decrement the time. If [%%], set time relative to total (percentage)\n"
	       "	vol [+/-]VOL - Set the volume to VOL. If [+/-], increment/decrement the volume.\n"
	       "	next [N] - Play next music. If N, jump to N songs ahead\n"
	       "	prev [N] - Play the previous song, if N is specified, jump to N songs behind\n"
	       "	playlist [FILE/PATH] - Prints the playlist. If [FILE/PATH], REPLACES the current playlist with the one from the given PATH or FILE. \n"
	       "	mute - Toggle mute\n"
	       "	repeat - Toggle repeat current music\n"
	       "	format \";FORMAT;\" - Returns the string formatted accordingly, with information from the currently playing music\n"
	       "	add [PATH]... - Appends each PATH to the playlist\n"
	       "	rem [POS]... - Removes the music at the given POS in the playlist\n"
	       "	stat - Returns a status list of the current music\n"
	       "	curr - Returns ';artist; - ;title;' of the current music.\n"
	       "	clear - Clears the playlist\n"
	       "	shuf - Shuffles the playlist\n"
	       "FLAGS \n"
	       "	-s SOCKET_PATH - Set socket path to SOCKET_PATH (must be specified before command)"
	       "	-c COMMAND - Sends a custom command to all plugins"
	       "	--cm COMMAND - Sends a custom command to mpv-catchme lua plugin"
	       "	-h - prints this message"
	       "EXAMPLES\n"
	       "	catchme format ';artist; - ;title; [;path;]'\n"
	       "	catchme playlist ~/music/\n"
	       "	catchme add ~/music/cardcaptor_sakura/*\n"
	       "	catchme -c write-playlist #(requires mpv-catchme mpv lua plugin)\n"
	       "	catchme seek 50%%\n"
	       "	catchme vol -10\n"
	       "OBS\n"
	       "  partial commands are valid as long they're not ambiguous, e.g. shuf=shuffle, tog=toggle, vol=volume,\n"
	       "  play=play, playl=playlist, playlist-p=playlist-play\n"
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

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		die("failed to create socket");

	remote.sun_family = AF_UNIX;
	strncpy(remote.sun_path, socket_path, 108);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(fd, (struct sockaddr *)&remote, len) == -1)
		die("failed to connect to socket");
}

// send 'msg' to socket, writes the response to 'result'
static char *send_to_socket(char *msg, char *result)
{
	int t;
	while (true) {
		if (send(fd, msg, strlen(msg), 0) == -1)
			die("error: send");

		if ((t = recv(fd, result, DATABUF_SIZE - 1, 0)) > 0) {
			result[t] = '\0';
			return result;
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
	if (send_to_socket(cmdbuff, databuff)) {
		struct json_object *res = json_tokener_parse(databuff);
		const char *error = json_object_get_string(
			json_object_object_get(res, "error"));
		if (error != NULL && strncmp(error, "success", 7) == 0) {
			const char *str = json_object_get_string(
				json_object_object_get(res, "data"));
			strncpy(result, str, result_size - 1);
			result[strlen(str)] = '\0';
			return true;
		}
		json_object_put(res);
	}
	result[0] = '\0';
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

void cm_playlist_clear(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_CLEAR);
	send_to_socket(cmdbuff, cmdbuff);
}

void cm_add_many(char *path[], int start, int count)
{
	for (int i = start; i < count; i++) {
		cm_add(path[i]);
	}
}

void cm_add(const char *path)
{
	if (path[0] != '/')
		die("doesn't look like a full path");
	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_APPEND, path);
	send_to_socket(cmdbuff, cmdbuff);
}

void cm_playlist(const char *path)
{
	if (path[0] != '/') {
		const char *cwd = getcwd(databuff, DATABUF_SIZE);
		strncpy(databuff, cwd, DATABUF_SIZE - 1);
		strncat(databuff, "/", 2);
		strncat(databuff, path, DATABUF_SIZE - 1);
		snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_LOAD, databuff);
	} else
		snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_LOAD, path);
	send_to_socket(cmdbuff, cmdbuff);
	msleep(500);
}

void cm_remove(const int id)
{
	int rem = 0;
	get_property_int("playlist-pos", &rem);

	int playlist_count = 0;
	get_property_int("playlist-count", &playlist_count);

	if (id >= playlist_count) {
		fprintf(stderr,
			"given index %d is outside of the playlist range 0-%d\n",
			id, playlist_count - 1);
		exit(EXIT_FAILURE);
	}

	snprintf(cmdbuff, SOCKETBUF_SIZE, PLAYLIST_REMOVE, id);
	send_to_socket(cmdbuff, cmdbuff);
	if (id == rem)
		msleep(500);
}

void cm_repeat(void)
{
	bool loop = false;
	get_property_bool("loop-file", &loop);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "loop-file",
		 loop ? "no" : "inf");
	send_to_socket(cmdbuff, cmdbuff);
}

void cm_mute(void)
{
	bool mute = false;
	get_property_bool("mute", &mute);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "mute",
		 mute ? "no" : "yes");
	send_to_socket(cmdbuff, cmdbuff);
}

void cm_toggle(void)
{
	bool pause = false;
	get_property_bool("pause", &pause);

	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause",
		 pause ? "no" : "yes");
	send_to_socket(cmdbuff, cmdbuff);
}

void cm_play(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause", "no");
	send_to_socket(cmdbuff, cmdbuff);
}

void cm_volume(const char *vol)
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

void cm_seek(const char *seek)
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

void cm_print_playlist(void)
{
	int playlist_len = 0;
	get_property_int("playlist-count", &playlist_len);

	for (int i = 0; i < playlist_len; i++) {
		if (get_filepath(databuff, i)) {
			printf("%s\n", databuff);
		}
	}
}

void cm_play_index(const int index)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PLAYING_MSG, index);
	send_to_socket(cmdbuff, cmdbuff);
	msleep(100);
}

void cm_status(void)
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

	int playlist_cnt = 0;
	get_property_int("playlist-count", &playlist_cnt);
	playlist_cnt--;

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
	       artist, title, status ? "paused" : "playing", pos, playlist_cnt,
	       0.0, 0.0, percent_pos, speed, vol, mute, databuff);
}

// there's a lot of room for improvement
void cm_format(char *format)
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

void cm_current(void)
{
	char *result = get_artist_title();
	printf("%s", result);
	free(result);
}

void cm_pause(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SET_PROPERTY_MSG, "pause", "yes");
	send_to_socket(cmdbuff, cmdbuff);
}

void cm_next(const int n)
{
	if (n == 1)
		send_to_socket(PLAYLIST_NEXT, databuff);
	else if (n == -1)
		send_to_socket(PLAYLIST_PREV, databuff);
}

void cm_shuffle(void)
{
	snprintf(cmdbuff, SOCKETBUF_SIZE, SHUFFLE_PLAYLIST_MSG);
	send_to_socket(cmdbuff, databuff);
	msleep(500);
}

int handle_custom_function(bool catchme, int argc, char *argv[], int index)
{
	if (catchme) {
		snprintf(cmdbuff, SOCKETBUF_SIZE, SCRIPT_CUSTOM_COMMAND_CATCHME,
			 argv[index]);
	} else {
		snprintf(cmdbuff, SOCKETBUF_SIZE, SCRIPT_CUSTOM_COMMAND,
			 argv[index]);
	}
	send_to_socket(cmdbuff, databuff);
	return index;
}

static int get_consecutive_path_count(int count, int first_path_index,
				      char *paths[])
{
	int result = 0;
	for (int i = first_path_index; i < count; i++) {
		if (paths[i][0] == '/')
			result++;
		else
			break;
	}
	return result;
}

int main(int argc, char *argv[])
{
	int n;
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "tog", 3) == 0) { // tog/toggle
			open_socket();
			cm_toggle();
		} else if (strncmp(argv[i], "vol", 3) == 0) { // vol/volume
			i++;
			if (i == argc) {
				fprintf(stderr,
					"vol requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			}
			open_socket();
			cm_volume(argv[i]);
		} else if (strncmp(argv[i], "next", 4) == 0) {
			i++;
			if (i == argc) {
				// if no argument, next music
				open_socket();
				cm_next(1);
			} else if (get_int(argv[i], &n)) {
				// otherwise, jump to 'current + n'
				open_socket();
				cm_next(n);
			} else {
				fprintf(stderr, "invalid integer for next\n");
				return EXIT_FAILURE;
			}
		} else if (strncmp(argv[i], "prev", 4) == 0) { // prev/previous
			i++;
			if (i == argc) {
				// if no argument, previous music
				open_socket();
				cm_next(-1);
			} else if (get_int(argv[i], &n)) {
				// otherwise, jump to 'current - n'
				open_socket();
				cm_next(-n);
			} else {
				fprintf(stderr, "invalid integer for prev\n");
				return EXIT_FAILURE;
			}
		} else if (strncmp(argv[i], "seek", 4) == 0) {
			i++;
			if (i == argc) {
				fprintf(stderr,
					"seek requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			}
			open_socket();
			cm_seek(argv[i]);
		} else if (strncmp(argv[i], "curr", 4) == 0) { // curr/current
			open_socket();
			cm_current();
		} else if (strncmp(argv[i], "play", 5) == 0) {
			i++;
			if (i == argc) {
				// if no argument, unpauses
				open_socket();
				cm_play();
			} else if (get_int(argv[i], &n)) {
				// otherwise, play the given valid integer
				open_socket();
				cm_play_index(n);
			} else {
				fprintf(stderr, "invalid integer for play\n");
				return EXIT_FAILURE;
			}
		} else if (strncmp(argv[i], "format", 6) == 0) {
			i++;
			if (i == argc) {
				fprintf(stderr,
					"format requires 1 string parameter, none given.\n");
				return EXIT_FAILURE;
			}
			open_socket();
			cm_format(argv[i]);
		} else if (strncmp(argv[i], "pause", 5) == 0) {
			open_socket();
			cm_pause();
		} else if (strncmp(argv[i], "shuf", 4) == 0) { // shuf/shuffle
			open_socket();
			cm_shuffle();
		} else if (strncmp(argv[i], "stat", 4) == 0) { // stat/status
			open_socket();
			cm_status();
		} else if (strncmp(argv[i], "mute", 4) == 0) {
			open_socket();
			cm_mute();
		} else if (strncmp(argv[i], "rem", 3) == 0) { // rem/remove
			i++;
			if (i == argc) {
				fprintf(stderr,
					"remove requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			} else if (!get_int(argv[i], &n)) {
				fprintf(stderr, "invalid integer for remove\n");
				return EXIT_FAILURE;
			}
			open_socket();
			cm_remove(n);
		} else if (strncmp(argv[i], "add", 3) == 0) {
			i++;
			if (i == argc) {
				fprintf(stderr, "no path given.\n");
				return EXIT_FAILURE;
			}
			open_socket();
			int count = get_consecutive_path_count(argc, 2, argv);
			cm_add_many(argv, i, count);
			i += count - 1;
		} else if (strncmp(argv[i], "clear", 5) == 0) {
			open_socket();
			cm_playlist_clear();
		} else if (strncmp(argv[i], "repeat", 6) == 0) {
			open_socket();
			cm_repeat();
		} else if (strncmp(argv[i], "playlist", 5) == 0) {
			i++;
			open_socket();
			if (i == argc)
				cm_print_playlist();
			else
				cm_playlist(argv[i]);
		} else if (strncmp(argv[i], "--cm", 4) == 0) {
			open_socket();
			i = handle_custom_function(true, argc, argv, ++i);
			return EXIT_SUCCESS;
		} else if (strncmp(argv[i], "-c", 2) == 0) {
			open_socket();
			i = handle_custom_function(false, argc, argv, ++i);
			return EXIT_SUCCESS;
		} else if (strncmp(argv[i], "-h", 2) == 0) {
			usage();
			return EXIT_SUCCESS;
		} else if (strncmp(argv[i], "-s", 2) == 0) {
			i++;
			if (i == argc) {
				fprintf(stderr,
					"-s requires 1 parameter, none given.\n");
				return EXIT_FAILURE;
			}
			if (fd >= 0)
				close(fd);
			fd = -1;
			strncpy(socket_path, argv[i], 107);
			socket_path[strlen(socket_path)] = '\0';
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

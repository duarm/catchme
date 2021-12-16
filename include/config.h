// catchme conf

#define VERSION "1.0.0"
#define SOCKETBUF_SIZE 512
#define DATABUF_SIZE 1024
#define MAX_PATH_SIZE 512
#define CONFIG_HOME "/home/sakura/.config/"

// music_name_cache will have this format
// useful if you're using the music_path_cache to search for musics
// NOT YET IMPLEMENTED
#define NAME_FORMAT ";id;: ;artist; - ;title; ;genre; ;album;"
// if artist or title is missing, it will fallback to this format
// ;id; and ;filename; are always available
// NOT YET IMPLEMENTED
#define FALLBACK_NAME_FORMAT ";id;: ;filename;"

// we cant set the volume to be greater than this
#define MAX_VOLUME 120

// path to the cache file where we store the path of each music in the current playlist.
// can be overwritten with -p
char music_path_cache[MAX_PATH_SIZE] = CONFIG_HOME "/catchme/music_path_cache";
// path to the cache file where we store the name of each music in the current playlist.
// can be overwritten with -n
char music_names_cache[MAX_PATH_SIZE] = CONFIG_HOME "/catchme/music_name_cache";

// socket paths are capped at 108 bytes
char socket_path[108] = CONFIG_HOME "/catchme/catchme-socket";

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


// catchme conf

#define VERSION "0.8.0"
#define MUSIC_DIR "/mnt/files/music/"
#define MUSIC_PATHS_CACHE "/home/sakura/.config/catchme/music_path_cache"
#define MUSIC_NAMES_CACHE "/home/sakura/.config/catchme/music_name_cache"
#define SOCKETBUF_SIZE 512
#define DATABUF_SIZE 512

#define MAX_PATH_SIZE 512
char socket_path[MAX_PATH_SIZE] = "/home/sakura/.config/catchme/catchme-socket";

#define SET_PLAYING_MSG \
	"{ \"command\": [\"set_property\", \"playlist-pos\", %d ] }\n" // index
#define LOOP_FILE_MSG \
	"{ \"command\": [\"set_property_string\", \"loop-file\", \"%s\" ] }\n" // loop (inf, no)
#define SET_PROPERTY_MSG \
	"{ \"command\": [\"set_property\", \"%s\", \"%s\"] }\n" // property, parameter
#define GET_PROPERTY_MSG \
	"{ \"command\": [\"get_property\", \"%s\"] }\n" // property
#define PLAYLIST_APPEND \
	"{ \"command\": [\"loadfile\", \"%s\", \"append\" ] }\n" // file
#define PLAYLIST_CLEAR \
	"{ \"command\": [\"playlist-clear\"] }\n"
#define PLAYLIST_REMOVE \
	"{ \"command\": [\"playlist-remove\", \"%d\"] }\n" // index
#define GET_PROPERTY_STRING_MSG \
	"{ \"command\": [\"get_property_string\", \"%s\"] }\n" // property
#define GET_PLAYLIST_FILENAME_MSG \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/filename\"] }\n" // index
#define GET_PLAYLIST_ARTIST_MSG \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/artist\"] }\n" // index
#define GET_PLAYLIST_TITLE_MSG \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/title\"] }\n" // index
#define SHUFFLE_PLAYLIST_MSG \
	"{ \"command\": [\"playlist-shuffle\" ] }\n"
#define SET_VOLUME_MSG \
	"{ \"command\": [\"set_property\", \"volume\", %d ] }\n" // volume
#define SEEK_PERCENTAGE_MSG \
	"{ \"command\": [\"set_property\", \"percent-pos\", %f ] }\n" // percentage
#define SEEK_MSG \
	"{ \"command\": [\"set_property\", \"playback-time\", %f ] }\n" // time
#define METADATA_MSG \
	"{ \"command\": [\"get_property\", \"metadata/by-key/%s\" ] }\n" // metadata


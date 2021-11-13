// catchme conf

#define VERSION "0.8.0"
#define SOCKET_FILE "/home/sakura/.config/catchme/catchme-socket"
#define MUSIC_DIR "/mnt/files/music/"
#define MUSIC_PATHS_CACHE "/home/sakura/.config/catchme/music_path_cache"
#define MUSIC_NAMES_CACHE "/home/sakura/.config/catchme/music_name_cache"
#define SOCKETBUF_SIZE 512
#define DATABUF_SIZE 512

#define SET_PLAYING_MSG \
	"{ \"command\": [\"set_property\", \"playlist-pos\", %d ] }\n"
#define PLAY_MSG \
	"{ \"command\": [\"set_property\", \"pause\", \"no\"] }\n"
#define PAUSE_MSG \
	"{ \"command\": [\"set_property\", \"pause\", \"yes\"] }\n"
#define TOGGLE_MSG \
	"{ \"command\": [\"set_property\", \"pause\", \"%s\"] }\n"
#define SET_PROPERTY_MSG \
	"{ \"command\": [\"set_property\", \"%s\", \"%s\"] }\n"
#define GET_PROPERTY_MSG \
	"{ \"command\": [\"get_property\", \"%s\"] }\n"
#define GET_PROPERTY_STRING_MSG \
	"{ \"command\": [\"get_property_string\", \"%s\"] }\n"
#define GET_PLAYLIST_FILENAME_MSG \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/filename\"] }\n"
#define GET_PLAYLIST_ARTIST_MSG \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/artist\"] }\n"
#define GET_PLAYLIST_TITLE_MSG \
	"{ \"command\": [\"get_property_string\", \"playlist/%d/title\"] }\n"
#define SHUFFLE_PLAYLIST_MSG \
	"{ \"command\": [\"playlist-shuffle\" ] }\n"
#define NEXT_PLAYLIST_MSG \
	"{ \"command\": [\"set_property\", \"playlist-pos\", %d ] }\n"
#define PREV_PLAYLIST_MSG \
	"{ \"command\": [\"set_property\", \"playlist-pos\", %d ] }\n"
#define SET_VOLUME_MSG \
	"{ \"command\": [\"set_property\", \"volume\", %d ] }\n"
#define SEEK_PERCENTAGE_MSG \
	"{ \"command\": [\"set_property\", \"percent-pos\", %f ] }\n"
#define SEEK_MSG \
	"{ \"command\": [\"set_property\", \"playback-time\", %f ] }\n"
#define METADATA_MSG \
	"{ \"command\": [\"get_property\", \"metadata/by-key/%s\" ] }\n"


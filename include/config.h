// smpd (sakura's music player daemon) conf

#define VERSION "0.8.0"
#define SOCKET_FILE "/home/sakura/.config/smpd/socket"
#define MUSIC_DIR "/mnt/files/music/"
#define MUSIC_CACHE_DIR "/home/sakura/.config/smpd/music_cache"
#define SOCKETBUF_SIZE 512
#define DATABUF_SIZE 512

// protocol
#define SERVER_COMMAND \
	"mpv --really-quiet --loop-playlist=inf --idle=yes --input-ipc-server="SOCKET_FILE" "MUSIC_DIR

#define PLAY_MSG \
	"{ \"command\": [\"set_property\", \"pause\", \"no\"] }\n"
#define PAUSE_MSG \
	"{ \"command\": [\"set_property\", \"pause\", \"yes\"] }\n"
#define TOGGLE_MSG \
	"{ \"command\": [\"set_property\", \"pause\", \"%s\"] }\n"
#define GET_PROPERTY_MSG \
	"{ \"command\": [\"get_property\", \"%s\"] }\n"
#define SHUFFLE_PLAYLIST_MSG \
	"{ \"command\": [\"playlist-shuffle\" ] }\n"
#define NEXT_PLAYLIST_MSG \
	"{ \"command\": [\"set_property\", \"playlist-pos\", %d ] }\n"
#define PREV_PLAYLIST_MSG \
	"{ \"command\": [\"set_property\", \"playlist-pos\", %d ] }\n"
#define VOLUME_MSG \
	"{ \"command\": [\"set_property\", \"volume\", %d ] }\n"
#define SEEK_PERCENTAGE_MSG \
	"{ \"command\": [\"set_property\", \"percent-pos\", %f ] }\n"
#define SEEK_MSG \
	"{ \"command\": [\"set_property\", \"playback-time\", %f ] }\n"
#define METADATA_MSG \
	"{ \"command\": [\"get_property\", \"metadata/by-key/%s\" ] }\n"


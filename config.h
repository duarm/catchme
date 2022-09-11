// catchme conf

#define VERSION "0.9.1"
#define SOCKETBUF_SIZE 512
#define DATABUF_SIZE 1024
#define CONFIG_HOME "/home/sakura/.config/catchme/"
// we cant set the volume to be greater than this
#define MAX_VOLUME 120

// path to the cache file where we store the path of each music in the current playlist.
// can be overwritten with -p
char music_path_cache[] = CONFIG_HOME "music_path_cache";
// path to the cache file where we store the name of each music in the current playlist.
// can be overwritten with -n
char music_names_cache[] = CONFIG_HOME "music_name_cache";

// socket paths are capped at 108 bytes
char socket_path[108] = CONFIG_HOME "catchme-socket";

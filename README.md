# Catch you, Catch me (catchme)

Catch You, Catch me is a cli interface to communicate with a mpv server
through an unix socket, written in pure and simple c99.

## More About
I made this because mpvc was super slow due to being a shell script.
This program doesn't have any runtime dependencies, as everything it uses (musl as libc, json-c)
was static linked. This results in faster loading times, especially important in a program which
runs for some milliseconds and then closes.

The default protocol communicates with mpv and was only tested with mpv.
I tried to make it as easy as possible to adapt to mpv's evolving protocol,
but it might be possible to communicate with other servers by editing each message on config.h.

## Build & Install

### Arch
```shell
pacman -S base-devel git mpv musl
git clone https://gitlab.com/kurenaiz/catchme.git
cd catchme/
make && sudo make install
```

Or

```shell
yay -S catchme-git
```

Or use the provided PKGBUILD.

## Configuration

You'll need to start your music server first.

### MPV

First, edit the SOCKET_FILE macro in config.h to point to the socket file. On our case, it will be located
at $XDG_CONFIG_HOME/catchme/catchme-socket

```c
// catchme/include/config.h

// dont forget the quotes
#define SOCKET_FILE "/home/USER/.config/catchme/catchme-socket"
```
Recompile and install.

You can, alternatively, alias the catchme command with the -s flag passing the path to the socket.

Now, create a script to start the mpv server and make it executable.
The important option is the --input-ipc-server and --video=no, the rest is up to you, 
some recommended ones:

```shell
# ./start_catchme
#!/bin/sh

# you can add your own options here
exec mpv --really-quiet --video=no --loop-playlist=inf --idle=yes \
        --input-ipc-server=$XDG_CONFIG_HOME/catchme/catchme-socket \
        /path/to/my/musics
```
You can customize this script to reflect the start state of your mpv server.

```shell
# ./start_catchme
#!/bin/sh

# mpv now will be paused on startup
exec mpv --pause --really-quiet --video=no --loop-playlist=inf --idle=yes \
        --input-ipc-server=$XDG_CONFIG_HOME/catchme/catchme-socket \
        /path/to/my/musics
```

```shell
# ./start_catchme
#!/bin/sh

# mpv will now start playing the last playlist you played
musics="path/to/my/musics"
[ -e "$XDG_CONFIG_HOME/catchme/music_path_cache" ] && \
	musics="--playlist=$XDG_CONFIG_HOME/catchme/music_path_cache"

exec mpv --pause --really-quiet --video=no --loop-playlist=inf --idle=yes \
        --input-ipc-server=$XDG_CONFIG_HOME/catchme/catchme-socket \
	$musics
```

Execute this file on your start script.

You can now communicate with your mpv server through catchme. See Usage section below.

## Usage
usage: catchme [-s SOCKET_PATH] [-p PATHS_CACHE] [-n NAMES_CACHE] [-vl [-h] COMMAND
```shell
$ catchme play 4 # changes current music to the one at index 4
$ catchme play # unpauses
$ catchme next # plays next song (index 5)
$ catchme vol -10 # decrease volume by 10
$ catchme vol 80 # set vol to 80
$ catchme seek +10 # seek +10 seconds
$ catchme seek 50% # seek to the middle of the music
$ catchme format ";artist; - ;title; [;status;]" # return formatted info about the current song
$ catchme mute # mutes
$ catchme mute # unmutes
$ catchme toggle # pauses
$ catchme toggle # unpauses
...
```

each catchme instance binds to one socket, this means that this doesn't work:
```shell
$ catchme toggle -s "$SOCKET2" toggle
```
the workaround is to call catchme again:
```shell
$ catchme toggle && catchme -s "$SOCKET2" toggle
```
-n and -p are different, since we just write to the given file.
```shell
# writes the paths and names to $NAMES_CACHE and $PATHS_CACHE, then writes just the names to $HOME/names
$ catchme -n "$NAMES_CACHE" -p "$PATHS_CACHE" write -p "$HOME/names" write
```


### Commands
- play [POS] - Unpauses, if POS is specified, plays the music at the given POS in the playlist.

- pause - Pauses

- toggle - Toggle pause

- seek `[+/-]TIME[%]` - Increments `[+]`, decrements `[-]` or sets the absolute time of the current music, you can also put
  at the end to seek to that percentage

- vol/volume `[+/-]VOL` - Increments `[+]`, decrements `[-]` or sets the absolute value of volume

- current/curr - Returns the artist and title of the current song in the ;artist; - ;title;" format, if any of those metadatas are missing
  returns the filename instead.

- next [N] - Play next music, if N is specified, jump to N songs ahead

- previous/prev [N] - Play the previous song, if N is specified, jump to N songs behind

- playlist FILE/PATH - REPLACES the current playlist with the one from the given PATH or FILE

- print-playlist - Prints the whole playlist to stdout

- mute - Toggle mute

- repeat - Toggle repeat current music

- add FILE/PATH - Appends the file or a file list of paths to the current playlist

- remove POS - Removes the music at the given POS from the playlist

- shuffle/shuf - Shuffles the playlist

- status - Returns a status list of the current music

- format "FORMAT" - Returns the string formatted accordingly, with information from the currently playing music (see Format below)

- clear - Clears the playlist

- idle - TODO

- write [path/name] - Writes to music_names_cache if 'name' is specified, to music_paths_cache if 'path', otherwise write to both.

###OBS
partial commands are valid as long they're not ambiguous, e.g. shuf=shuffle, tog=toggle, vol=volume, play=play, playl=playlist, playlist-p=playlist-play

### Format
;name;, ;title;, ;artist;, ;album;, ;album-artist;,
;genre;, ;playlist-count;, ;playlist-pos;, ;percent-pos;,
;status;, ;volume;, ;muted;

TODO
;path;, ;single;, ;time;, ;precise-time;, ;speed;, ;length;, ;remaining;, ;repeat;

#### Why ";"
I wanted three things. 1) something which would not interfere with most other programs. 2) no multikey. 3) keep mpv names.

both "%" and `"$"` would violate 2) and 1), `"$"` would interfere with shell and `"%"` with c.
`"-"` would interfere with 3), as -album-artist- would confuse -album- and -artist-.

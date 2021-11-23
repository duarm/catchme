# Catch you, Catch me (catchme)

Catch You, Catch me is a cli interface to communicate with the mpv server
through an unix socket, written in pure and simple c99. I made this because
mpvc was super slow due to being a shell script.
The default protocol communicates with mpv and was only tested with mpv.
I tried to make it as easy as possible to adapt to mpv's evolving protocol,
but it might be possible to communicate with other servers.

## Features

- [ ] playlists
- [X] basic control (see commands section)

## Build & Install

### Arch
```shell
pacman -S base-devel git mpv musl
git clone https://gitlab.com/kurenaiz/catchme.git
cd catchme/
make && sudo make install
```

Or

todo package managers

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
The important option is the --input-ipc-server, the rest is up to you, some recommended ones:

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
[ -e "$XDG_CONFIG_HOME/catchme/music_path_cache" ] && \
	musics="--playlist=$XDG_CONFIG_HOME/catchme/music_path_cache" || \
	musics="path/to/my/musics"

exec mpv --pause --really-quiet --video=no --loop-playlist=inf --idle=yes \
        --input-ipc-server=$XDG_CONFIG_HOME/catchme/catchme-socket \
	$musics
```

Execute this file on your start script.

You can now communicate with your mpv server through catchme.

```shell
$ catchme toggle
$ catchme next
...
```

## Commands
- play - Unpauses

- pause - Pauses

- toggle - Toggle pause

- seek \[+/-\]TIME - Increments \[+\], decrements \[-\] or sets the absolute time of the current music

- vol/volume \[+/-\]VOL - Increments \[+\], decrements \[-\] or sets the absolute value of volume

- current/curr - Returns the name of the current music

- next - Plays next music

- prev - Plays previous music

- play-index ID - plays the music the the given ID

- playlist - Prints the whole playlist to stdout

- playlist-play FILE/PATH - REPLACES the current playlist with the one from the given PATH or FILE (analagous to the --playlist)

- mute - Toggle mute

- repeat - Toggle repeat current music

- add FILE/PATH - Appends the file/file list/path to the current playlist

- remove ID - Removes the music at the given ID from the playlist

- status - Returns a status list of the current music ?REMOVE?

- format PATTERN - TODO

- clear - Clears the playlist

- idle - TODO

- update - Updates the music_names_cache and music_paths_cache

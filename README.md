# Catch you, Catch me (catchme)

Catch You, Catch me is a cli interface to communicate with the mpv server
through an unix socket, written in pure and simple c99. I made this because
mpvc was super slow due to being a shell script.
The default protocol communicates with mpv and was only tested with mpv.
I tried to make it as easy as possible to adapt to mpv's evolving protocol,
but it might be possible to communicate with other servers.

## Features

- [ ] playlists
- [X] cli

## Build & Install

```shell
pacman -S mpv
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
$ catchme next
$ catchme toggle
...
```

### Protocol

Each command is a macro in the config.h file. This makes it easier
to adapt to different protocols or changes to the default one (mpv)

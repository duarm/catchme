# Catch you, Catch me (catchme)

Catch You, Catch me is a cli interface to communicate with a mpv server
through an unix socket, written in pure and simple c99.

## More About
I made this because mpvc was super slow due to being a shell script.
This program doesn't have any runtime dependencies, as everything it uses (musl as libc, json-c)
was static linked. This project was created for my personal use and I found it useful enough to share.

## Build & Install
The recommended way to install this package is to keep a copy of it by cloning the repository, copying
config.def.h to config.h, and customizing it to your liking. You'll run `git pull` if you wish to update to the most recent version.

### Arch
```shell
$ pacman -S base-devel git mpv musl
$ git clone https://gitlab.com/kurenaiz/catchme.git
$ cd catchme/
# edit config.h as specified in the Configuration section below
$ make && sudo make install # compile and install
```

Or

```shell
yay -S catchme-git
```

Or use the provided PKGBUILD.

## Configuration
### catchme
First, edit the SOCKET_FILE macro in config.h to point to the socket file which will be created by mpv's ipc server. 
On our case, it will be located at $XDG_CONFIG_HOME/catchme/catchme-socket.

If you installed from the PKGBUILD/AUR, you can skip
this part, since the PKGBUILD already sets this up to $XDG_CONFIG_HOME/catchme/ automatically.

```c
// catchme/config.h

// dont forget the quotes
#define SOCKET_FILE "/home/USER/.config/catchme/catchme-socket"
```

There are some other options you can customize here like MAX_VOLUME, once you're finished
compile and install. You must install with elevated privileges.
```shell
$ make && sudo make install
```

You can, alternatively, alias the catchme command with the -s flag passing the path to the socket.

### mpv
Create a script to start the mpv server and make it executable.
The important option is the --input-ipc-server and --video=no, the rest is up to you, 
some recommended ones:

```shell
#!/bin/sh

# you can add your own options here
exec mpv --really-quiet --video=no --loop-playlist=inf --idle=yes \
        --input-ipc-server=$XDG_CONFIG_HOME/catchme/catchme-socket \
        /path/to/my/musics
```
You can customize this script to reflect the start state of your mpv server.

```shell
#!/bin/sh

# mpv now will be paused on startup
exec mpv --pause --really-quiet --video=no --loop-playlist=inf --idle=yes \
        --input-ipc-server=$XDG_CONFIG_HOME/catchme/catchme-socket \
        /path/to/my/musics
```

```shell
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

## Usage & Examples
usage: catchme `[-s SOCKET_PATH]` `[-p PATHS_CACHE]` `[-n NAMES_CACHE]` `[-v]` `[-h]` COMMAND
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
# clears the playlist, then starts playing another playlist, then shuffle playlist, then write to names/paths cache
$ catchme clear && catchme playlist /path/to/my/playlist && catchme shuff && catchme write
$ tagutil $(catchme format ";path;")
# /home/sakura/music/sailor_moon/La_Soldier.opus
---
- title: La Soldier
- artist: Sailor Moon
- singer: Moon Lips
- album: Sailor Moon OST
$ catchme format ";artist; - ;title; (;album;) [;status;]"
Sailor Moon - La Soldier (Sailor Moon OST) [paused]
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
- add FILE/PATH - Appends each file, or file containing a list of files, to the playlist
- remove POS - Removes the music at the given POS from the playlist
- shuffle/shuf - Shuffles the playlist
- status - Returns a status list of the current music
- format ";FORMAT;" - Returns the string formatted accordingly, with information from the currently playing music (see Format below)
- clear - Clears the playlist
- write [path/name] - Writes to music_names_cache if 'name' is specified, to music_paths_cache if 'path', otherwise write to both.
 
Partial commands are valid as long they're not ambiguous, e.g. shuf=shuffle, tog=toggle, vol=volume, play=play, playl=playlist, playlist-p=playlist-play

### Format
title, artist, album, album-artist,
genre, playlist-count, playlist-pos, percent-pos,
status, volume, mute, path, loop-file, speed

### Quirks
When you run certain commands like 'play', 'add' or 'shuff', you might notice that catchme takes some seconds to exit,
even though the command already finished executing.
That happens because these commands forces mpv into an audio-reconfig state which needs to be waited and handled correctly,
Since I'm lazy and never done a proper sync with these events coming from the socket, I just sleep for a few miliseconds
after sending the command to mpv. This hopefully will change in the future.

The `'write'` command is never called automatically, you might want to call every time you make a change to playlist if you're
using 'music_names_cache' to select music in catchmenu for example.

#### Why ";"
I wanted three things. 1) something which would not interfere with most other programs. 2) no multikey. 3) keep mpv names.

both `"%"` and `"$"` would violate 2) and 1), `"$"` would interfere with shell and `"%"` with c.
`"-"` would interfere with 3), as -album-artist- would confuse -album- and -artist-.

after careful consideration I was left with `'.'` and `','`, since I couldn't choose, I ended up with `';'`.

### Todo

- NAME_FORMAT to specify the format to write to music_names_cache
- Properly syncing with 'audio-reconfig' events
- FORMAT - time, precise-time, length, remaining
- add/remove more than one at a time (catchme add ~/msc1.mp3 ~/msc2.mp3 ~/msc3.mp3)


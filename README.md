# Catch you, Catch me (catchme)

Catch me is a CLI interface to communicate with a mpv ipc socket, written in pure and simple c99.

## More About
I made this because mpvc was super slow due to being a shell script, and I wanted to use mpv as a 
music player daemon.

The idea is to use this as a controller to mpv, mpv would be the daemon and server. This only sends 
a command, and mpv reacts. Out of the box you have all of mpv default commands at your disposal, 
but you can extend its functionality through a [mpv script](https://gitlab.com/kurenaiz/mpv-catchme) 

## Usage & Examples
usage: catchme `[-s SOCKET_PATH]` `[-p PATHS_CACHE]` `[-n NAMES_CACHE]` `[-v]` `[-h]` `[-c]` COMMAND
```shell
$ catchme play 2 # changes current music to the one at index 2
$ catchme play # unpauses
$ catchme vol -10 # decrease volume by 10
$ catchme vol 80 # set vol to 80
$ catchme seek +10 # seek +10 seconds
$ catchme seek 50% # seek to the middle of the music
$ catchme add /home/sakura/sailor_moon/* # adding all musics on sailor_moon folder, obs: this makes use of 
shell glob, /* is not a feature of catchme
$ catchme rem 2 # removes the music at position 2 in the playlist
$ catchme rem $(($(catchme format ";playlist-count;")-1)) # removes the last music from the playlist
$ catchme next # plays next song (index 3)
$ catchme prev # plays next song (index 2)
$ catchme next 3 # jump 3 songs (index 5)
$ catchme mute # mutes
$ catchme mute # unmutes
$ catchme toggle # pauses
$ catchme toggle # unpauses
$ catchme repeat # loop current music
$ catchme repeat # stop looping
# clears the playlist, then starts playing another playlist, then shuffle playlist, then write to names/paths file
$ catchme clear && catchme playlist /home/sakura/music/ && catchme shuff && catchme -c write-playlist
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
- **play** [POS] - Unpauses, if POS is specified, plays the music at the given POS in the playlist.
- **pause** - Pauses
- **toggle** - Toggle pause
- **seek** `[+/-]TIME[%]` - Increments `[+]`, decrements `[-]` or sets the absolute time of the 
current music, you can also put
  at the end to seek to that percentage
- **vol/volume** `[+/-]VOL` - Increments `[+]`, decrements `[-]` or sets the absolute value of volume
- **current/curr** - Returns the artist and title of the current song in the ;artist; - ;title;" format,
if any of those metadatas are missing
  returns the filename instead.
- **next [N]** - Play next music, if N is specified, jump to N songs ahead
- **previous/prev** [N] - Play the previous song, if N is specified, jump to N songs behind
- **playlist** FILE/PATH - REPLACES the current playlist with the one from the given PATH or FILE
. If no PATH/FILE given, print playlist
- **mute** - Toggle mute
- **repeat** - Toggle repeat current music
- **add** FILE/PATHS - Appends each file, or file containing a list of files, to the playlist
- **remove** POS - Removes the music at the given POS from the playlist
- **shuffle/shuf** - Shuffles the playlist
- **status** - Returns a status list of the current music
- **format** ";FORMAT;" - Returns the string formatted accordingly, with information from the currently
playing music (see Format below)
- **clear** - Clears the playlist
- **CUSTOMCOMMANDS** - (see Custom Commands below)

Partial commands are valid as long they're not ambiguous, e.g. shuf=shuffle, vol=volume, play=play,
playl=playlist

### Format
title, artist, album, album-artist,
genre, playlist-count, playlist-pos, percent-pos,
status, volume, mute, path, loop-file, speed

### Custom Commands
you can send custom commands to mpv with the `-c` option, the `--cm` options sends commands directly to
the `mpv-catchme` script.

```shell
$ catchme --cm write-playlist
```


## Build & Install
The recommended way to install this package is to keep a copy of it by cloning the repository, copying
config.def.h to config.h, and customizing it to your liking. You'll run `git pull` if you wish to update 
to the most recent version.

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
$ yay -S catchme-git
```

Or use the provided PKGBUILD.


We have a prebuilt json-c library at lib/ for the sake of easy of building without having a runtime dependency 
(since arch does not provide static libraries).

## Configuration
### catchme
You just need to start mpv with an ipc server, and tell catchme where's the socket file located, 
step-by-step below.

First, edit the SOCKET_FILE macro in config.h to point to the socket file which will be created by 
mpv's ipc server. On our case, it will be located at $XDG_CONFIG_HOME/catchme/catchme-socket.

If you installed from the PKGBUILD/AUR, you can skip
this part, since the PKGBUILD already sets this up to $XDG_CONFIG_HOME/catchme/ automatically.

```c
// catchme/config.h

// dont forget the quotes
#define SOCKET_FILE "/home/USER/.config/catchme/catchme-socket"
```

There are some other options you can customize here like MAX_VOLUME, once you're finished
compile and install.
```shell
$ make && sudo make install
```

You can, alternatively, alias the catchme command with the -s flag passing the path to the socket.

### mpv
Create a script to start the mpv server and make it executable.

You can customize this script to reflect the start state of your mpv server.
```shell
#!/bin/sh

# you can add your own options here, check the mpv documentation, some good ones
# --pause, start paused
# --shuffle, shuffles the playlist
# --volume=X, starts with volume set to X
mpv --really-quiet --video=no --idle=yes --load-scripts=no \
        --pause \ 
        --input-ipc-server=$XDG_CONFIG_HOME/catchme/catchme-socket \
        --loop-playlist=inf 
        /path/to/my/musics
```

Use your scripting power to do cool stuff.

Now just execute this file to start the server, place this on your start script to auto run when booting up.

See wiki section for Quirks, Why ";" for format, Todo, and more.

### Extensions
[catchmenu](https://gitlab.com/kurenaiz/catchmenu) - dmenu script for selecting music

[mpv-catchme](https://gitlab.com/kurenaiz/mpv-catchme) - catchme mpv script

[catchme-scripts](https://gitlab.com/kurenaiz/catchme-scripts) - general scripts which use catchme

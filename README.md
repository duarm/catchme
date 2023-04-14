# Catch you, Catch me (catchme)

Catch me is a CLI interface to communicate with a mpv ipc socket, written in pure and simple c99.

## More About
I made this because mpvc was super slow due to being a shell script, and I wanted to use mpv as a 
music player daemon.

The idea is to use this as a controller to mpv, mpv would be the daemon and server. This only sends 
a command, and mpv reacts. Out of the box you have all of mpv default commands at your disposal, 
but you can extend its functionality through a [mpv script](https://gitlab.com/kurenaiz/mpv-catchme) 

## Usage & Examples
usage: catchme `[-s SOCKET_PATH]` `[-h]` `[-c]` `[--cm]` COMMAND
```shell
$ catchme start # executes the start script, which opens mpv
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
$ catchme clear && catchme playlist /home/sakura/music/ && catchme shuff && catchme --cm write-playlist
$ tagutil $(catchme format ";path;")
# /home/sakura/music/sailor_moon/La_Soldier.opus
---
- title: La Soldier
- artist: Sailor Moon
- singer: Moon Lips
- album: Sailor Moon OST
$ catchme format ";artist; - ;title; (;album;) [;status;]"
Sailor Moon - La Soldier (Sailor Moon OST) [paused]
$ catchme curr
Sailor Moon - La Soldier
$ catchme status
Sailor Moon - La Soldier (Sailor Moon OST)
[playing] #212/614 0.00/0.00 (12%)
speed: 1.00x volume: 60% muted: 0 loop: no
$ catchme stop # closes mpv
```

### Perks
- Partial commands are valid as long they're not ambiguous, e.g. shuf=shuffle, vol=volume, play=play,
playl=playlist

- `catchme start` forwards all parameters to mpv, so you can do something like `catchme start --volume=10 --af="lavfi=[loudnorm=I=-16:TP=-3:LRA=4]"` and it will work.

- Catchme can execute multiple commands in one call, they happen in order. e.g. `catchme seek 50% toggle` is equivalent to `catchme seek 50% && catchme toggle`

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
### Arch
```shell
# make method
$ pacman -S base-devel git mpv musl
$ git clone https://gitlab.com/kurenaiz/catchme.git
$ cd catchme/
$ make && sudo make install # compile and install
```

Or

```shell
# PKGBUILD method
$ git clone https://gitlab.com/kurenaiz/catchme.git
$ cd catchme/
$ makepkg -si
```

Or

```shell
# AUR method
$ yay -S catchme-git
# OR
$ paru -S catchme-git
```

We have a prebuilt json-c library at lib/ for the sake of easy of building without having a runtime dependency 
(since arch does not provide static libraries), I might want to switch to json.h later.

## Configuration
You just need to start mpv with an ipc server, catchme provides a start script ready to use, 
if you want to customize, checkout the step-by-step below.

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
        "$@" \
        --loop-playlist=inf 
        /path/to/my/musics
```

Catchme forwards all parameters to the script when starting the server through `catchme start`, so if you place "$@" like the script above, you can do this:
```shell
$ catchme start --shuffle --volume=70
```

Use your scripting power to do cool stuff.

See wiki section for Quirks, Todo, and more.

### Extensions
[catchmenu](https://gitlab.com/kurenaiz/catchmenu) - dmenu script for selecting music

[mpv-catchme](https://gitlab.com/kurenaiz/mpv-catchme) - catchme mpv script

[catchme-scripts](https://gitlab.com/kurenaiz/catchme-scripts) - general scripts which use catchme

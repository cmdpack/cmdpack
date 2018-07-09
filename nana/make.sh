#!/bin/bash
cflag=$(sdl-config --cflags --libs)
cmd="gcc -o2 -o nana.out sdl.c fileopen.c nana.c $cflag"

echo ">>> $cmd"
$cmd

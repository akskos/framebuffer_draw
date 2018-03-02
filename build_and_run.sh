#!/bin/sh

make
if [ $? -eq 0 ]; then
    tput civis
    sudo ./bin/fb $IMAGEPATH
    tput cnorm
fi

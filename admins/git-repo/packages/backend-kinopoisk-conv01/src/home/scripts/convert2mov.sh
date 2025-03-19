#!/usr/bin/env bash

INFILE=$1
OUTFILE=$2

FFMPEG="/usr/local/bin/ffmpeg"
BT=`$FFMPEG -i $INFILE 2>&1|grep bitrate|cut -d, -f 3|cut -dx -f 1|awk '{print $2}'`

$FFMPEG -i $INFILE -f mp4 -vcodec libx264 -b $BT"k" -acodec copy -y $OUTFILE 2>&1 > /dev/null

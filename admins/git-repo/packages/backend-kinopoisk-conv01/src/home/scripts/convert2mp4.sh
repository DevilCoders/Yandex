#!/usr/bin/env bash

INFILE=$1
OUTFILE=$2
FFMPEG="/usr/local/bin/ffmpeg"
QTFAST="/usr/bin/qt-faststart"
BT=`$FFMPEG -i $INFILE 2>&1 | grep bitrate | cut -d, -f 3 | cut -dx -f 1 | awk '{print $2}'`

# ffmpeg
$FFMPEG -i $INFILE -b $BT"k" -f mp4 -vcodec libx264 -acodec libfaac -ab 128k -ac 2 -ar 48000 $OUTFILE-1 2>&1 > /dev/null
# qt-fast
$QTFAST $OUTFILE-1 $OUTFILE-2 2>&1 > /dev/null
# rename
mv $OUTFILE-2 $OUTFILE
# clean
rm -f $OUTFILE-1

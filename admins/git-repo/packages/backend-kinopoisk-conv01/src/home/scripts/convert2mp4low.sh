#!/usr/bin/env bash

INFILE=$1
OUTFILE=$2
WIDTH=$3
BT=$4

FFMPEG="/usr/local/bin/ffmpeg"
QTFAST="/usr/bin/qt-faststart"

WW=`$FFMPEG -i $INFILE 2>&1|grep Video|cut -d, -f 3|cut -dx -f 1|awk '{print $1}'|perl -ne '/[\d\.]+/ && print $&'`
WH=`$FFMPEG -i $INFILE 2>&1|grep Video|cut -d, -f 3|cut -dx -f 2|awk '{print $1}'|perl -ne '/[\d\.]+/ && print $&'`

WWW=`echo $WW|awk '{print $1}'`
WHH=`echo $WH|awk '{print $1}'`

WWC=`echo "scale=1; $WWW/$WIDTH" | bc | awk '{print $1}'`
WHC=`echo "scale=0; $WHH/$WWC" | bc`
CLC=`echo "scale=1; $WHC/4"| bc | awk -F "." '{print $2}' `
CLC2=`echo "scale=0; $WHC/4"| bc`
CLC3=`echo "$CLC2*4" | bc`

        if echo $CLC | grep -q "0" ; then

        $FFMPEG -i $INFILE -s "$WIDTH"x"$WHC" -b $BT"k" -f mp4 -vcodec libx264 -acodec libfaac -ab 128k -ac 2 -ar 48000 $OUTFILE-1 2>&1 > /dev/null
        else
        $FFMPEG -i $INFILE -s "$WIDTH"x"$CLC3" -b $BT"k" -f mp4 -vcodec libx264 -acodec libfaac -ab 128k -ac 2 -ar 48000 $OUTFILE-1 2>&1 > /dev/null
fi

$QTFAST $OUTFILE-1 $OUTFILE-2 2>&1 > /dev/null
mv $OUTFILE-2 $OUTFILE
rm -f $OUTFILE-1

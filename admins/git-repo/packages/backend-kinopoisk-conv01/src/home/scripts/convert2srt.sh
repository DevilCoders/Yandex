#!/usr/bin/env bash

TRACK_INFO=`ffmpeg -i $1 2>&1|grep Subtitle`

if echo $TRACK_INFO | grep -q "Subtitle"; then

TRACK_NUMBER=`ffmpeg -i $1 2>&1|grep Subtitle|awk '{print $2}'|awk -F ":" '{print $2}'|head -n 1|perl -ne '/[\d\.]+/ && print $&'`
TRACK=`echo $TRACK_NUMBER+1|bc`

	MP4Box -rem $TRACK $1
	ttxml2srt.py $2 $2.srt
#	MP4Box -add $1 -add $2.srt:hdlr=sbtl:lang=rus:font=Serif:size=18 -new $2.tmp
        MP4Box -add $1#video -add $1#audio:lang=eng -add $2.srt:hdlr=sbtl:lang=rus:font=Serif:size=18 -new $2.tmp
	mv $2.tmp $1
	rm -f $2.srt

else
	ttxml2srt.py $2 $2.srt
#	MP4Box -add $1 -add $2.srt:hdlr=sbtl:lang=rus:font=Serif:size=18 -new $2.tmp
        MP4Box -add $1#video -add $1#audio:lang=eng -add $2.srt:hdlr=sbtl:lang=rus:font=Serif:size=18 -new $2.tmp
	mv $2.tmp $1
	rm -f $2.srt
fi

#!/usr/bin/env bash

INFILE=$1
OUTFILE=$2
WIDTH=480

FFMPEG="/usr/local/bin/ffmpeg"

WW=`$FFMPEG -i $INFILE 2>&1|grep Video|cut -d, -f 3|cut -dx -f 1|awk '{print $1}'|grep -v title|grep -v handler_name`
WH=`$FFMPEG -i $INFILE 2>&1|grep Video|cut -d, -f 3|cut -dx -f 2|awk '{print $1}'|grep -v title|grep -v handler_name`

WWC=`echo "scale=1; $WW/$WIDTH" | bc | awk '{print $1}'`
WHC=`echo "scale=0; $WH/$WWC" | bc`
CLC=`echo "scale=1; $WHC/4"| bc | awk -F "." '{print $2}' `
CLC2=`echo "scale=0; $WHC/4"| bc`
CLC3=`echo "$CLC2*4" | bc`

        if echo $CLC | grep -q "0" ; then

        $FFMPEG -i $INFILE -s "$WIDTH"x"$WHC" -b 1024k -f mp4 -vcodec libx264 -flags +loop+mv4 -cmp 256 -partitions +parti4x4+parti8x8+partp4x4+partp8x8+partb8x8 -subq 7 -trellis 1 -refs 5 -bf 0 -me_range 16 -g 250 -keyint_min 25 -sc_threshold 40 -i_qfactor 0.71 -qmin 10 -qmax 51 -qdiff 4 -acodec libfaac -ab 128k -ac 2 -ar 48000 $OUTFILE-1 2>&1 > /dev/null
        echo "H ok"
        else
        $FFMPEG -i $INFILE -s "$WIDTH"x"$CLC3" -b 1024k -f mp4 -vcodec libx264 -flags +loop+mv4 -cmp 256 -partitions +parti4x4+parti8x8+partp4x4+partp8x8+partb8x8 -subq 7 -trellis 1 -refs 5 -bf 0 -me_range 16 -g 250 -keyint_min 25 -sc_threshold 40 -i_qfactor 0.71 -qmin 10 -qmax 51 -qdiff 4 -acodec libfaac -ab 128k -ac 2 -ar 48000 $OUTFILE-1 2>&1 > /dev/null
        echo "H change"
fi

# qt-fst
/usr/bin/qt-faststart $OUTFILE-1 $OUTFILE-2 2>&1 > /dev/null
rm -f $OUTFILE-1
echo "qt-faststart ok"

# rename
mv $OUTFILE-2 $OUTFILE-3.mp4

# handbrake
/usr/bin/HandBrakeCLI -O -i $OUTFILE-3.mp4 -o $OUTFILE 2>&1 > /dev/null
rm -f $OUTFILE-3.mp4

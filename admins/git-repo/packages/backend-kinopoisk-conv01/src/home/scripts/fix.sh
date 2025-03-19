#!/usr/bin/env bash

MP4FILE="${1}"
TMPFILE="${1}.tmp.mov"
FFMPEG="/usr/local/bin/ffmpeg"
QTFAST="/usr/bin/qt-faststart"

if $FFMPEG -i "${MP4FILE}" 2>&1 > /dev/null|grep Stream|grep Video|grep -q h264 ; then
    if $FFMPEG -i "${MP4FILE}" 2>&1 > /dev/null|grep Stream|grep Audio:|egrep -i -q 'mp3|lame|aac'; then
       if $FFMPEG -i "${MP4FILE}" 2>&1 > /dev/null|grep -q Data ; then
         #echo "Additional data track!"
          $FFMPEG -i "${MP4FILE}" -vcodec copy -acodec copy "${TMPFILE}" 2>&1 > /dev/null  || rm -f ${TMPFILE}
          if [[ `stat -c%s "${TMPFILE}"` -lt 100000  ]] ; then
                #echo "ERROR: result file too small"
                rm -f "${TMPFILE}"
                exit 10
          fi
          #echo "Index!"
          mv "${TMPFILE}" "${MP4FILE}"
          $QTFAST "${MP4FILE}" "${TMPFILE}" 2>&1 > /dev/null
          mv "${TMPFILE}" "${MP4FILE}" 2>&1 | grep -v cannot
          exit 0
       elif  $FFMPEG -i "${MP4FILE}" 2>&1 > /dev/null|grep -F 'Stream #0.0'| grep -q Audio ; then
          #echo "Wrong video track order!"
                    $FFMPEG -i "${MP4FILE}" -vcodec copy -acodec copy "${TMPFILE}" 2>&1 > /dev/null  || rm -f ${TMPFILE}
          if [[ `stat -c%s "${TMPFILE}"` -lt 100000  ]] ; then
                #echo "ERROR: result file too small"
                rm -f "${TMPFILE}"
                exit 10
          fi
          mv "${TMPFILE}" "${MP4FILE}"
          $QTFAST "${MP4FILE}" "${TMPFILE}" 2>&1 > /dev/null
          mv "${TMPFILE}" "${MP4FILE}" 2>&1 | grep -v cannot
          exit 0
       fi
      #echo "Already good"
          $QTFAST "${MP4FILE}" "${TMPFILE}" 2>&1 > /dev/null
          mv "${TMPFILE}" "${MP4FILE}" 2>&1 | grep -v cannot
      exit 0
    else
      $FFMPEG -i "${MP4FILE}" -vcodec copy -acodec libfaac -ac 2 -ab 128k -ar 48000 "${TMPFILE}" 2>&1 > /dev/null  || rm -f ${TMPFILE}
      if [[ `stat -c%s "${TMPFILE}"` -lt 100000  ]] ; then
            #echo "ERROR: result file too small"
            rm -f "${TMPFILE}"
            exit 10
      fi
          mv "${TMPFILE}" "${MP4FILE}"
          $QTFAST "${MP4FILE}" "${TMPFILE}" 2>&1 > /dev/null
          mv "${TMPFILE}" "${MP4FILE}" 2>&1 | grep -v cannot
      exit 0
    fi
else
  #echo NOT a H264
  exit 11
fi

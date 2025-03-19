#!/bin/bash

STAT_FILE="/tmp/index_stat.txt"
EMAIL_SRC="$1"
EMAIL_SUBJECT="$2"
EMAIL_DST="$3"

rm -f "$STAT_FILE"
/usr/bin/mongodb_index_usage > "$STAT_FILE"

if [ -s "$STAT_FILE" ]
then
    echo "Index statistic" | mail -a "$STAT_FILE" -E -r "$EMAIL_SRC" -s "$EMAIL_SUBJECT" "$EMAIL_DST"
fi
rm -f "$STAT_FILE"


#!/usr/bin/env bash

#
# Gets all images that have been modified last hour and puts them to s3.
#

LOG=/var/log/s3/upload_s3_hourly_sync.log

function s3_upload () {
    src_name="$1"
    dst_s3="$src_name"
    if [ -f "$dst_s3" ]
    then
        # Converts symlink to original name
        dst_s3=$(readlink -f "$dst_s3")
        dst_s3=${dst_s3##/home/www/kinopoisk.ru/}
        dst_s3=${dst_s3##/home/www/static.kinopoisk.ru/}
        /usr/local/bin/upload_s3_static.py "$src_name" "$dst_s3"
    fi
}

echo "$(date) Start local run." >> $LOG
echo "$(date) View info about uploading every image in /var/log/s3/upload_s3-static.log" >> $LOG


images=$(find /home/www/kinopoisk.ru/images/ -type f -mmin -65)

echo -e "$(date) $(echo -e "$images" | wc -l) Images found: \n$images"  >> $LOG

for im in $images; do
    s3_upload $im
    sleep 0.1 # Not to make burst traffic on s3.
done


echo "$(date) Called s3_upload function for all those images. View /var/log/s3/upload_s3-static.log for more info."
echo "$(date) Finish run." >> $LOG


# EOF


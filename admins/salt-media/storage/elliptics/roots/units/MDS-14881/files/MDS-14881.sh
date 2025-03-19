#!/bin/bash

if [ -e '/var/tmp/MDS-14881.tskv' ]
then
    rm /var/tmp/MDS-14881.tskv
fi


cd /var/log/karl

for file in karl*
do
    echo "$file" | grep -q ".zst$"
    if [ $? -eq 0 ]
    then
        new_file=`echo "$file" | sed 's/.zst//g'`
        zstd -d -f $file
        /usr/bin/MDS-14881.py $new_file 123 >> /var/tmp/MDS-14881.tskv || exit 4
        rm $new_file
    else
        /usr/bin/MDS-14881.py $file 123 >> /var/tmp/MDS-14881.tskv || exit 4
    fi
 
done

touch /var/tmp/MDS-14881.done

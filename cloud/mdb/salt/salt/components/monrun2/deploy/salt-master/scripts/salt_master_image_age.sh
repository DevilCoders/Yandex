#!/usr/bin/env bash
if [ ! -f "/etc/cron.yandex/update_salt_image.py" ]
then
    echo "0;s3 image is disabled"
    exit 0
fi

/etc/cron.yandex/update_salt_image.py --quiet check $*

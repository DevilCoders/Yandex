#!/bin/sh

EXCLUDE_DISTRIBUTIONS="mirrors-apt.osdyson.org|archlinux-arm$|fedora-linux|point|launchpad|asplinux-tigro|ftp.flightgear.org|naulinux|tank|deepin|\.ping|^mirrors$"
INCLUDE_DISTRIBUTIONS="mirrors-deepin-packages mirrors-deepin-releases 
                       mirrors-point-packages mirrors-point-releases
                       mirrors-naulinux-NauLinux mirrors-naulinux-SLCE
                       mirrors-ftp.flightgear.org-flightgear
                       mirrors-ftp.flightgear.org-simgear"

DISTRIBUTIONS_PATH="/mirror/fedora/ /mirror/ /mirror/mirrors/ /mirror/archlinux-arm /mirror/mirrors/launchpad"

DISTRIBUTIONS="$(find $DISTRIBUTIONS_PATH -maxdepth 1 -mindepth 1 -type d  | \
		sed 's@^/mirror/@@g;s@/@-@g' | egrep -v "$EXCLUDE_DISTRIBUTIONS") \
		$INCLUDE_DISTRIBUTIONS"

for dist in $DISTRIBUTIONS; do
    if [ "$(/usr/bin/distribution-up-to-date.sh $dist 2> /dev/null)" != "0;ok" ]; then
        NOT_SYNCED="$NOT_SYNCED $dist"
    fi
done

if [ "x$NOT_SYNCED" == "x" ]; then
    echo "0;ok"
else
    echo "1;$NOT_SYNCED"
fi

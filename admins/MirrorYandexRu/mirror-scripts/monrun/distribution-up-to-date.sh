#!/bin/sh

DISTRIBUTION="$1"

if [ "$DISTRIBUTION" == "mirrors-deepin-packages" ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ "$DISTRIBUTION" == "fedora-secondary" ]; then
    true
elif [ "$DISTRIBUTION" == "mirrors-deepin-releases" ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ "$DISTRIBUTION" == "mirrors-naulinux-NauLinux" ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ "$DISTRIBUTION" == "mirrors-naulinux-SLCE" ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ "$DISTRIBUTION" == "mirrors-point-packages" ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ "$DISTRIBUTION" == "mirrors-point-releases" ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ "$DISTRIBUTION" == "mirrors-ftp.flightgear.org-flightgear" ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ "$DISTRIBUTION" == "mirrors-ftp.flightgear.org-simgear" ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ $(echo $DISTRIBUTION | grep launchpad) ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@-@/@g')
elif [ $(echo $DISTRIBUTION | grep archlinux-arm) ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@arm-@arm/@g')
elif [ $(echo $DISTRIBUTION | grep "^mirrors-" ) ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@^mirrors-@mirrors/@g')
elif [ $(echo $DISTRIBUTION | grep "^fedora-" ) ]; then
    DISTRIBUTION=$(echo $DISTRIBUTION | sed 's@^fedora-@fedora/@g')
fi

if [ $(echo $DISTRIBUTION | grep "archlinux-arm") ]; then
    TIMESTAMP="sync"
else
    TIMESTAMP=".mirror.yandex.ru"
fi

EPS="60"

if [ "x$HOST" == "x" ]; then
    HOST="pull-mirror.yandex.net"
fi

localtime()
{
    LOCALTIME=$(date +%s --date "$(cat $DISTDIR/$TIMESTAMP)" 2> /dev/null || \
	date +%s --date "@$(cat $DISTDIR/$TIMESTAMP)")

}

remotetime()
{
    REMOTETIME=$(date +%s --date "$(curl -s http://$HOST/$DISTRIBUTION/$TIMESTAMP)" 2> /dev/null || \
	date +%s --date "@$(curl -s http://$HOST/$DISTRIBUTION/$TIMESTAMP)")
}


if [ "x$DISTRIBUTION" == "x" ]; then
    echo "Distribution is not defined. Abort..."
    exit 2
fi

DISTDIR="$MAINSYNCDIR/$DISTRIBUTION"

if [ ! -f "$DISTDIR/$TIMESTAMP" ]; then
    echo "2;$DISTRIBUTION never synced yet"
else
    localtime
    remotetime
    
    if [ $(($REMOTETIME-$LOCALTIME)) -gt $EPS ]; then
        if [ $(($(date +%s)-$REMOTETIME)) -gt 21600 ]; then 
            if [ ! -f $DISTDIR ]; then
                echo "2; $DISTRIBUTION unsynced"
            else
                echo "0;ok"
            fi
        else
            echo "0;ok"
        fi
    else
        echo "0;ok"
    fi
fi

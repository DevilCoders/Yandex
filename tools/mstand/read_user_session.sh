#!/bin/sh
if [ -z "$1" ]; then
    echo "Please specify usersession path. Examples:"
    echo './read_user_session.sh //user_sessions/pub/search/daily/2016-06-01/clean'
    echo './read_user_session.sh //user_sessions/pub/search/daily/2016-01-02/clean["y0000000000000000000":"y9000000000000000000"] | head -n 20000 > user_session_sample'
    exit 1
fi

if [ "x$2" = "x-t" ]; then
    yt --proxy hahn read "$1" '--format=yamr'
else
    yt --proxy hahn read "$1" '--format=<encode_utf8=false>json'
fi

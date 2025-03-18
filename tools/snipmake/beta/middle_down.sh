#!/usr/local/bin/bash

ME=mark4
MS=middlesearch_$ME
HOST=scrooge.yandex.ru
MY_PATH=/var/tmp/sky_snippets_$ME


echo "Down middle: start"
sky run "cd $MY_PATH && ./kill.sh $MS && rm -Rf $MY_PATH" +$HOST
echo "Down middle: done"

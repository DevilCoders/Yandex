#!/bin/bash

count=`for i in \`seq 1 15\` ; do jhttp.sh -n api-yaru.yandex.ru -u /ping-test/ ; done | grep 'ok'  | wc -l` ;
if [ $count -gt 10 ] ; then echo "0; ok" ; else echo "2;  api-yaru.yandex.ru problem" ; fi

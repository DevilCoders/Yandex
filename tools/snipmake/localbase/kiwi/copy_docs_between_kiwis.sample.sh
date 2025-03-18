#!/usr/bin/env bash

set -e -o pipefail

# Old process

#pv -l urls.txt | \
#    bin/kwworm read \
#    -k 50 --maxrps 20 -f protobin -Q ../local_export.txt -p full \
#    > local_export.protobin

#bin/kwworm -c scrooge.search.yandex.net --port 32068 --maxrps 100 write \
#    -k 50 -d -f protobin -b TRUNK -s -n -g -i local_export.protobin

# New process

pv -l urls.txt | bin/kiwi-transfer -d scrooge -t 37402 -b TRUNK


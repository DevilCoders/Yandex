#!/usr/local/bin/bash

ARCADIA_PATH="../../../../"

rm newindexdir

if [ -f arc2dir ] || [ -f newindexarc ]
then
    ./arc2dir newindex
else
    echo "ERROR: arc2dir doesn't exist"
    exit 1
fi

if [ -f mergearchive ]
then
    ln -s -f ../newindexarc yandex/oldbd000arc
    ln -s -f ../newindexdir yandex/oldbd000dir
    ./mergearchive -h . -k 000 -u urlmenu.trie
else
    echo "ERROR: mergearchive doesn't exist"
    exit 1
fi


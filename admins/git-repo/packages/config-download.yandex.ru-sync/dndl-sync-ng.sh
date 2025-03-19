#!/bin/bash
sync_www () {
    rsync -avvp --delete download1-622.yandex.ru::dnld /var/www/ --perms --chmod=ugo=rwx || rsync -avvp --delete download1.yandex.ru::dnld /var/www/ --perms --chmod=ugo=rwx
}

sync_block () {
    rsync -avvp --delete download1-622.yandex.ru::blck /opt/block/ --perms --chmod=ugo=rwx || rsync -avvp --delete download1.yandex.ru::blck /opt/block/ --perms --chmod=ugo=rwx
}

sync_apache () {
    rsync -avvp --delete download1-622.yandex.ru::www.apache /opt/www.apache/ --perms --chmod=ugo=rwx || rsync -avvp --delete download1.yandex.ru::www.apache /opt/www.apache/ --perms --chmod=ugo=rwx
}

sync_htpasswd () {
    rsync -avvp --delete download1-622.yandex.ru::www.apache.htpasswd /etc/apache/htpasswd/ --perms --chmod=ugo=rwx || rsync -avvp --delete download1.yandex.ru::www.apache.htpasswd /etc/apache/htpasswd/ --perms --chmod=ugo=rwx
}

make_log () {
date +%s > /var/log/dndl-sync-ng.last
}

sync_all () {
sync_www && sync_block && sync_apache && sync_htpasswd && make_log
}

sync_all 1>/dev/null


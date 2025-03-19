#!/bin/bash

if [ -f /etc/mirror-sync.conf ]; then
    . /etc/mirror-sync.conf
else
    echo "Configuration file does not exist. Abort"
    exit 112
fi

REL_LOCAL_PATH="mirrors/ftp.linux.kiev.ua"
HOST="ftp.linux.kiev.ua"
MODULE="pub"

EXCLUDE="
--exclude=ALT/
--exclude=ArchLinux/
--exclude=CentOS/
--exclude=Calculate/
--exclude=Debian/
--exclude=Debian
--exclude=fedora/
--exclude=Fedora
--exclude=Knoppix
--exclude=Gentoo/
--exclude=Mageia/
--exclude=Mint/
--exclude=Slackware/
--exclude=SuSE/
--exclude=Ubuntu/
--exclude=deepin/
--exclude=openSUSE
--exclude=pointlinux/
--exclude=cygwin
--exclude=cygwin/
--exclude=mirrors/ftp.debian.org
--exclude=mirrors/ftp.altlinux.org
"

loadscripts
check4run
check4available
simple_sync
setstamp
compresslog

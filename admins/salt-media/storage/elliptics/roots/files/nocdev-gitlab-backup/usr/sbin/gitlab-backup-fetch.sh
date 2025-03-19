#!/bin/bash

BACKUP_DIR=/backup
PING_URL=http://localhost:81/ping-gitlab
STORE_COMMON_COUNT=7
STORE_REGISTRY_COUNT=1

###
LOG=/var/log/gitlab-backup-fetch.log

function log() {
	echo "$( date "+%Y-%m-%d-%H-%M-%S" ) $1"
}

(
	tty -s || {
		exec >$LOG
		exec 2>>$LOG
	}
	set -x
	
	[ "$(curl --fail --silent --write-out %{http_code} $PING_URL)" != "200" ] || exit 0
	log "Slave"

	if [ "$(curl --fail --silent --write-out %{http_code} http://noc-vcs-vla.yndx.net:81/ping-gitlab )" == "200" ]; then
		rsync_host=noc-vcs-vla.yndx.net
	elif [ "$(curl --fail --silent --write-out %{http_code} http://noc-vcs-sas.yndx.net:81/ping-gitlab )" == "200" ]; then
		rsync_host=noc-vcs-sas.yndx.net
	else
		log "Cannot detect master-gitlab host"
		exit 1
	fi
	
	renice +10 -p $$
	ionice -c 3 -p $$

	fail=no

	f=no
	RSYNC_PASSWORD="{{pillar['sec']['rsync-backup-password']}}" rsync -av --exclude='lost+found' rsync://noc-sync@${rsync_host}:874/backup/ /backup/ || f=yes
	[ "$f" == "yes" ] && fail=yes
	log "Rsync done (fail: $f)"

	(
		set -e
		cd $BACKUP_DIR
		ls -t | grep gitlab-common | tail -n+$((STORE_COMMON_COUNT+1)) | xargs --no-run-if-empty /bin/rm -f -v
		ls -t | grep gitlab-registry | tail -n+$((STORE_REGISTRY_COUNT+1)) | xargs --no-run-if-empty /bin/rm -f -v
	) || f=yes
	[ "$f" == "yes" ] && fail=yes
	log "Rotate done (fail: $f)"

	if [ "$fail" == "yes" ]; then
		exit 1
	else
		exit 0
	fi
)

if [ $? -ne 0 -a -f $LOG ]; then
	cat $LOG
fi

if [ -f $LOG ]; then
	/bin/mv -f $LOG ${LOG}.prev
fi


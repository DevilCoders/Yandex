#!/bin/bash

BACKUP_DIR=/backup
SRC_DIR=/data/gitlab
PING_URL=http://localhost:81/ping-gitlab
STORE_COMMON_COUNT=7
STORE_REGISTRY_COUNT=1

###
LOG=/var/log/gitlab-backup.log

function log() {
	echo "$( date "+%Y-%m-%d-%H-%M-%S" ) $1"
}

(
	tty -s || {
		exec >$LOG
		exec 2>>$LOG
	}
	set -x

	renice +10 -p $$
	ionice -c 3 -p $$

	[ "$(curl --fail --silent --write-out %{http_code} $PING_URL)" == "200" ] || exit 0
	log "Master"

	MAX_PROCS=$(expr `nproc` / 2 || echo 1)

	EXCLUDE_FILE=$SRC_DIR/backup.exclude
	EXCLUDE=
	if [ -f $EXCLUDE_FILE ]; then
		# sample:
		#  data/gitlab-rails/shared/artifacts
		#  data/gitlab-rails/shared/registry
		EXCLUDE="--exclude-from=$EXCLUDE_FILE"
	fi

	DATE=$( date "+%Y-%m-%d-%H-%M-%S" )
	fail=no

	# it seem's changed files cause non-zero exit code in tar
	#set -o pipefail

	# Create backup for postgresql only. Do not tar it.
	# path to store backup set in $SRC_DIR/data/gitlab-rails/etc/gitlab.yml
	f=no
	docker exec gitlab_web_1  gitlab-backup create SKIP=uploads,repositories,builds,artifacts,lfs,registry,pages,tar || f=yes
	[ "$f" == "yes" ] && fail=yes
	log "DB backup done (fail: $f)"

	f=no
	tar -C $SRC_DIR $EXCLUDE -c . | pzstd -p $MAX_PROCS > $BACKUP_DIR/gitlab-common-$DATE.tar.zst || f=yes
	[ "$f" == "yes" ] && fail=yes
	log "gitlab-common backup done (fail: $f)"

	if [ "$( date +%u )" == "4" -a -f $EXCLUDE_FILE ]; then
		f=no
		tar -C $SRC_DIR -c $( grep -v '^#' $EXCLUDE_FILE ) | pzstd -p $MAX_PROCS > $BACKUP_DIR/gitlab-registry-$DATE.tar.zst || f=yes
		[ "$f" == "yes" ] && fail=yes
		log "gitlab-registry backup done (fail: $f)"
	fi

	f=no
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


#!/bin/bash
set -eu

PROD_GROUP="nocdev-mysql"

BACKUP_DIR=$1
INC_DIR="$BACKUP_DIR/increment"
BASE_DIR="$BACKUP_DIR/base"
CACHE_FILE="/var/cache/mysql-resetup/state"
SSH_AGENT_VARS=~/.ssh/agent-vars


GOT_BACKUP=0
WITH_INC=0

log() {
	echo $(date) $1
}

log_info() {
	log " INFO $1"
}

log_error() {
	log " ERROR $1"
	exit 1
}

if [ ! -n "${SSH_AUTH_SOCK+a}" ];then
	if [ -n "$SUDO_USER" ];then
		REAL_HOME="$(getent passwd $SUDO_USER|cut -d : -f 6)"
	else
		REAL_HOME=$HOME
	fi

	if [ -f $SSH_AGENT_VARS ]; then
		. $SSH_AGENT_VARS
	fi

	if [ `LANG=C ssh-add -l 2>/dev/null | grep -v 'agent has no identities' | wc -l` -eq 0 ]; then
		ssh-add -l &>/dev/null | true
		if [ ${PIPESTATUS[0]} -eq 2 ]; then
			ssh-agent | sed 's/^echo/#echo/' > $SSH_AGENT_VARS || log_error "Failed to start ssh-agent" 
			. $SSH_AGENT_VARS
		fi
		ssh-add "$REAL_HOME/.ssh/id_rsa" || log_error "Failed to add keys"
	fi
fi

if [ "$(mysync info -s | awk '/master:/ {print $2}')" != "$(hostname -f)" ];then
	log_info "I'm not master. Exit";
	exit 0;
fi

for host in $(curl -s c.yandex-team.ru/api/groups2hosts/$PROD_GROUP|sort);do
	if ssh $host 'mysync state -s|grep `hostname -f`'|grep -v 'repl=master' > /dev/null;then
		last=$(ssh $host 'dirname $(ls /backup/*/xtrabackup_checkpoints|grep -v full|tail -n 1)')
		if [ -n "$last" ];then
			if [ ! -e "$CACHE_FILE" ] || [ $last != $(cat "$CACHE_FILE") ];then
				log_info "Getting incremental backup $last from $host..."
				rsync -a --delete --checksum "$host:$last/*" "$INC_DIR" || log_error "Failed to get incremental backup"
				WITH_INC=1
			else
				log_info "Already got newest version of incremental backup. Nothing to do."
				exit 0
			fi
		else
			log_info "Incremental backups not found."
		fi

		log_info "Getting base backup from $host..."
                rsync -a --delete --checksum $host:/backup/full/* "$BASE_DIR" || log_error "Failed to get base backup"
		log_info "Decompressing base backup..."
		xtrabackup --target-dir="$BASE_DIR" --decompress || log_error "Failed to decompress base backup."
		log_info "Preparing base backup..."
		xtrabackup --defaults-file=/etc/mysql/my.cnf --prepare --apply-log-only --target-dir="$BASE_DIR" || log_error "Failed to prepare base backup"
		GOT_BACKUP=1
		break
	fi

	if [ $GOT_BACKUP -eq 0]; then
		log_error "Could not get backup. Exit"
	fi
done

if [ $WITH_INC -eq 1 ]; then
	log_info "Decompressing incremental backup..."
	xtrabackup --target-dir="$INC_DIR" --decompress || log_error "Failed to decompress increment backup"
	log_info "Applying incremental backup..."
	xtrabackup --defaults-file=/etc/mysql/my.cnf --prepare --apply-log-only --target-dir="$BASE_DIR" --incremental-dir="$INC_DIR" || log_error "Failed to apply increment backup"
	echo $last > "$CACHE_FILE"
	rm -rf $INC_DIR
fi

log_info "Stopping MySQL server..."
systemctl stop mysql || log_error "Failed to stop MySQL server"

log_info "Moving backup to MySQL data directory..."
rm -rf /var/lib/mysql/*
xtrabackup --move-back --target-dir="$BASE_DIR" || log_error "Failed to move-back backup"
chown -R mysql:mysql /var/lib/mysql

log_info "Starting MySQL server..."
systemctl start mysql || log_error "Failed to start MySQL server"

for host in $(python3 -c "import yaml; print('\n'.join(yaml.safe_load(open('/etc/mysync.yaml'))['mysql']['hosts']))"|cut -d ':' -f 1);do
	if [ $host != $(hostname -f) ]; then
		log_info "Resetuping $host ..."
		ssh -A $host my-resetup --force || log_error "Failed to resetup $host"
	fi
done

if mysync state -s|grep ping|grep -vE 'repl=(master|running)'; then
	log_error 'Got problems with some hosts';
else
	log_info 'Done.';
	exit 0;
fi

#!/bin/bash
set -eu

PROD_GROUP="nocdev-mysql"
ADMIN_USER=$(grep ' user' /etc/mysync.yaml |awk '{ print $2 }')
ADMIN_PASSWD=$(grep ' password' /etc/mysync.yaml|awk '{ print $2 }')
REPL_USER=$(grep 'replication_user' /etc/mysync.yaml |awk '{ print $2 }')
REPL_PASSWD=$(grep 'replication_password' /etc/mysync.yaml|awk '{ print $2 }')

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

run_on_slaves() {
	for host in $(python3 -c "import yaml; print('\n'.join(yaml.safe_load(open('/etc/mysync.yaml'))['mysql']['hosts']))"|cut -d ':' -f 1);do
        	if [ $host != $(hostname -f) ]; then
                	log_info "Running '$1' on $host ..."
                	sudo ssh -o ForwardAgent=yes $host "$1" || log_error "Fail to run '$1' on $host"
        	fi
	done	
}

if [ "$(mysync info -s | awk '/master:/ {print $2}')" != "$(hostname -f)" ];then
	log_info "I'm not master. Exit";
	exit 0;
fi

if [ ! -n "${SSH_AUTH_SOCK+a}" ];then
        if [ -n "$SUDO_USER" ];then
                REAL_HOME="$(getent passwd $SUDO_USER|cut -d : -f 6)"
        else
                REAL_HOME=$HOME
        fi
        eval $(ssh-agent) > /dev/null && ssh-add "$REAL_HOME/.ssh/id_rsa" || log_error "Failed to start ssh-agent or add keys"
fi

log_info "Stopping mysync on master..."
sudo systemctl stop mysync || log_error 'Failed to stop mysync'

log_info "Stopping mysync on slaves..."
run_on_slaves 'sudo systemctl stop mysync'

log_info "Stopping MySQL..."
sudo /etc/init.d/mysql stop || log_error "Failed to stop mysql"

log_info "Resetuping master..."
SRC_NODES=$(curl -s "c.yandex-team.ru/api/groups2hosts/$PROD_GROUP"|tr '\n' ,)
sudo my-resetup --nodes $SRC_NODES --force --defaults-file /root/.my.cnf.production || log_error "Failed to resetup master"

log_info "Changing passwords..."
echo "ALTER USER '$REPL_USER' IDENTIFIED BY '"$REPL_PASSWD"';" | sudo mysql --defaults-file=/root/.my.cnf.production || log_error "Failed to change password for $REPL_USER"
echo "ALTER USER '$ADMIN_USER' IDENTIFIED BY '"$ADMIN_PASSWD"';" | sudo mysql --defaults-file=/root/.my.cnf.production || log_error "Failed to change password for $ADMIN_USER"
echo 'FLUSH TABLES;' | sudo mysql --defaults-file=/root/.my.cnf || log_error "Failed to flush tables"

log_info 'Starting mysync on master...'
sudo systemctl start mysync || log_error 'Failed to start mysync'

log_info "Resetuping slaves..."
run_on_slaves 'my-resetup --force'

log_info "Starting mysync on slaves..."
run_on_slaves 'sudo systemctl start mysync'

log_info "Done."

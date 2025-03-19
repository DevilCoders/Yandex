#!/bin/bash
## Provides ssl certs from secdist
SCRIPT_NAME=`basename $0`
USER="$1"
GROUP="$2"
SERVICE="$3"
FILES="$4"
PROJECT="$5"

# Variables
NGINXSSL="/etc/nginx/ssl"

function get_hosts {
	URL="http://c.yandex-team.ru/api/groups2hosts/$GROUP"
	curl -s --connect-timeout 1 $URL
}

function load_certs {
	MACHINE=$1
	FILE=$2
	ssh $MACHINE "export LC_USER='$USER' && secget ugc/$PROJECT/ssl/$SERVICE/$FILE > $NGINXSSL/$FILE && export LC_USER='' && chown root $NGINXSSL/$FILE && chmod 0600 $NGINXSSL/$FILE || exit 1" && echo "$MACHINE - done" || echo "$MACHINE - failed, please check if ForwardAgent enabled in config and ssh-add -L contains your key"
}

if [[ -z "$USER" ]] || [[ -z "$SERVICE" ]] || [[ -z "$FILES" ]] || [[ -z "$PROJECT" ]]; then
	echo "Usage: $SCRIPT_NAME [username] [condutor group] [service name] '[FILE1 FILE2 ... FILEn]' [project_name]"
	echo "e.g.: $SCRIPT_NAME aarseniev pdd-front pdd.yandex.ru pdd.pem hosting"
	exit 0
fi

for i in `get_hosts`; do
	for g in $FILES; do
		load_certs $i $g
	done
done

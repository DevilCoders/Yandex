#!/bin/bash
#
# autodetect root-email for host and write result to /etc/aliases;
#
HOSTNAME=$(hostname -f)
LOG_FILE="/var/log/autodetect-root-email.log"
ALIASES="/etc/aliases"
DEFAULT_EMAIL=$(grep 'root: ' "$ALIASES" | awk '{print $2}')
URL="http://c.yandex-team.ru/api/generator/agodin.host_mailto?fqdn="
FIRST="$1"
COLOR="\033[30m\033[47m"
CLEAR="\033[0m"


function randsleep()  {
sleep $(($RANDOM * 60 * 14 * 200 / 32767))
}

function log_message() {
    if [[ "$FIRST" = "configure" ]]; then
        echo -e "$1"
    else
        echo "[$(date +'%Y-%m-%d %H:%M:%S')] $1" >> "$LOG_FILE"
fi
}

function get_maillist() {
ROOT_EMAIL=$(curl -ks "$URL$HOSTNAME")
}

function retcode_handling() { 
RETCODE=$(curl -ks -w "\n%{http_code}"  "$URL$HOSTNAME" | tail -n1)
case $RETCODE in
    "200")
	get_maillist
        log_message "Received code "$RETCODE""
        ;;
    "404")
        log_message "Couldn't find project for host or host in conductor"
        exit 1
        ;;
    "500")
        log_message "Service temporarily unavailable"
        exit 1
        ;;
    *)
        log_message "Conductor returned error code "$RETCODE", exiting"
        exit 1
        ;;
esac
}

function append_email() {
    if [[ -z $DEFAULT_EMAIL ]]; then
        echo "root: $ROOT_EMAIL" >> $ALIASES
        log_message "Appended root email: $COLOR $ROOT_EMAIL $CLEAR"
    else
        return 1
    fi
}

function fix_email() {
    sed "s/$DEFAULT_EMAIL/$ROOT_EMAIL/g" -i $ALIASES
    log_message "Fixed root email from $DEFAULT_EMAIL to $COLOR $ROOT_EMAIL $CLEAR"
}

function check_email() {
        if [[ -z $ROOT_EMAIL ]]; then
        log_message "Root email variable is empty"
            exit 1
        else
            append_email || fix_email
        fi
}

function main () {
    retcode_handling
    if [[ -f $ALIASES ]]; then
        if [[ $(egrep -c $ROOT_EMAIL $ALIASES) == 1 ]]; then
            log_message "Root email is already set to $COLOR $ROOT_EMAIL $CLEAR"
        else
            check_email
        fi
        if [[ $ALIASES -nt ${ALIASES}.db ]]; then
            newaliases
        fi
    fi
}  

# MUSIC-19425
if test -e /etc/yandex-autodetect-root-email-stop; then
    exit 0
fi

if [[ "$1" = "configure" ]]; then
    main
else
    randsleep
    main
fi

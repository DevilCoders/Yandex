#! /usr/bin/env bash

CERT_DIR="/srv/certs"
CA_CONFIG="$CERT_DIR/ca.config"
CN=$1
TEAM=$2
EMAIL=$3
LOCK_FILE="$CERT_DIR/create.lock"
LOG_FILE="/var/log/yaroot/yaroot-create-p12.log"
OPENSSL=`which openssl`
LOCK_FILE="/tmp/yaroot-create-p12.lock"

function now_time() {
    date +"%Y-%m-%d %H:%M:%S"
}
function log_info() {
    echo -ne "`now_time` [INFO] $1" >> $LOG_FILE
}
function log_add() {
    echo -ne "\n`now_time` [INFO] $1" >> $LOG_FILE
}
function log_ok() {
    echo -ne " ok\n" >> $LOG_FILE
}
function log_done() {
    echo -ne "\n`now_time` [INFO] ...done\n" >> $LOG_FILE
}
function log_err() {
    echo -ne "\n`now_time` [ERR] $1 \n" >> $LOG_FILE
}

function check_args() {
    log_info "check arguments..."
    if [[ -z $CN ]] || [[ -z $TEAM ]] || [[ -z $EMAIL ]]; then
        log_err "arguments too low" && exit 1
    fi
    log_ok
}
check_args

#function check_local_lock() {
    #while `test -f $LOCK_FILE`; do
    #    sleep $(($RANDOM % 2))
    #done

    #echo $$ > $LOCK_FILE
#    flock -x -w 7200 $LOCK_FILE
#}
#check_local_lock

function check_lock() {
    log_info "check lock file..."
    while `/usr/lib/etcd/etcdctl get /create.lock 1>/dev/null 2>&1`; do
        log_add "locked from `/usr/lib/etcd/etcdctl get /create.lock`"
        sleep 2
    done
    log_done

    log_info "create lock file..."
    err_msg=$(/usr/lib/etcd/etcdctl set /create.lock $(hostname -f) --ttl 900 2>&1 1>/dev/null)
    if [[ $? != 0 ]]; then
        log_err "can't create lock file: $err_msg" exit 1
    fi
    sleep 1
    log_ok
}
#until [[ `/usr/lib/etcd/etcdctl get /create.lock 2>/dev/null` == `hostname -f` ]]; do
#    check_lock
#done

function remove_lock() {
    log_info "removing lock file..."
    rm -f $LOCK_FILE
    err_msg=`/usr/lib/etcd/etcdctl rm /create.lock 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't remove lock file: $err_msg"
    fi
    log_ok
}

function check_dirs() {
    log_info "check dirs..."
    for DIR in certs keys p12 requests; do
        if [[ ! -d $CERT_DIR/$DIR ]]; then
            log_err "directory $DIR not found" && remove_lock && exit 1
        fi
    done
    log_ok
}
check_dirs

function get_index() {
    log_info "get index files..."
    for index in index.txt index.txt.attr index.txt.attr.old index.txt.old serial serial.old; do
        if ! `/usr/lib/etcd/etcdctl get /$index 1>/dev/null 2>&1`; then
            log_err "can't get $index" && exit 1
        fi
    done
    for index in index.txt index.txt.attr index.txt.attr.old index.txt.old serial serial.old; do
        /usr/lib/etcd/etcdctl get /$index > $CERT_DIR/$index
    done
    log_ok
}
get_index

function create_key() {
    log_info "create private key for $CN..."
    err_msg=`$OPENSSL req -new -newkey rsa:2048 -nodes -keyout $CERT_DIR/keys/"$CN".key -subj "/C=RU/ST=Moscow/L=Moscow/O=root.yandex.com SelfSignedCA/OU=$TEAM/CN=$CN/emailAddres=$EMAIL" -out $CERT_DIR/requests/"$CN".csr 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't create private key for $CN: $err_msg" && remove_lock && exit 2
    fi
    log_ok
    
    log_info "chmod private key $CN..."
    err_msg=`chmod 600 $CERT_DIR/keys/"$CN".key 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't chmod private key $CN: $err_msg" && remove_lock && exit 2
    fi
    log_ok
}
create_key

function singing_request() {
    log_info "signing requests for $CN..."
    err_msg=`$OPENSSL ca -batch -config $CA_CONFIG -in $CERT_DIR/requests/"$CN".csr -out $CERT_DIR/certs/"$CN".crt 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't signing request for $CN: $err_msg" && remove_lock && exit 3
    fi
    log_ok

    log_info "remove request for $CN..."
    err_msg=`rm $CERT_DIR/requests/"$CN".csr 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't remove request for $CN: $err_msg" && remove_lock && exit 3
    fi
    log_ok
}
singing_request

function create_p12() {
    log_info "create PKCS#12 archive for $CN..."
    err_msg=`$OPENSSL pkcs12 -export -in $CERT_DIR/certs/"$CN".crt -inkey $CERT_DIR/keys/"$CN".key -out $CERT_DIR/p12/"$CN".p12 -passout pass: 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't create PKCS#12 archive for $CN: $err_msg" && remove_lock && exit 4
    fi
    log_ok
}
create_p12

function remove_keys() {
    log_info "remove keys for $CN..."
    #rm -f $CERT_DIR/certs/"$CN".crt
    rm -f $CERT_DIR/keys/"$CN".key
    log_ok
}
remove_keys

function create_config() {
    log_info "create openvpn config for $CN..."
    #BASE64_CERT=`$OPENSSL base64 -in $CERT_DIR/p12/"$CN".p12`
    cat $CERT_DIR/config > $CERT_DIR/p12/"$CN".config && echo "<ca>" >> $CERT_DIR/p12/"$CN".config && cat $CERT_DIR/RootCA.crt >> $CERT_DIR/p12/"$CN".config && echo "</ca>" >> $CERT_DIR/p12/"$CN".config && echo "<dh>" >> $CERT_DIR/p12/"$CN".config && cat $CERT_DIR/dh2048.pem >> $CERT_DIR/p12/"$CN".config && echo "</dh>" >> $CERT_DIR/p12/"$CN".config && echo "<tls-auth>" >> $CERT_DIR/p12/"$CN".config && cat $CERT_DIR/ta.key >> $CERT_DIR/p12/"$CN".config && echo "</tls-auth>" >> $CERT_DIR/p12/"$CN".config && echo "<pkcs12>" >> $CERT_DIR/p12/"$CN".config && $OPENSSL base64 -in $CERT_DIR/p12/"$CN".p12 >> $CERT_DIR/p12/"$CN".config && echo "</pkcs12>" >>$CERT_DIR/p12/"$CN".config
    if [[ $? != 0 ]]; then 
        log_err "can't create openvpn config for $CN" && remove_lock && exit 5
    fi
    log_ok
}
create_config

function chown_dir() {
    log_info "chown files..."
    chown -R corba:corba $CERT_DIR
    log_ok
}
chown_dir

function set_index() {
    log_info "set index files..."
    for index in index.txt index.txt.attr index.txt.attr.old index.txt.old serial serial.old; do
        if [[ ! -r $CERT_DIR/$index ]]; then
            log_err "can't read $index" && exit 1
        fi
    done
    for index in index.txt index.txt.attr index.txt.attr.old index.txt.old serial serial.old; do
        err_msg=$(/usr/lib/etcd/etcdctl set /$index "$(cat $CERT_DIR/$index)" 2>&1 1>/dev/null)
        if [[ $? != 0 ]]; then
            log_err "can't set $index: $err_msg"
        fi
    done
    log_ok
}
set_index
sleep 2

#remove_lock

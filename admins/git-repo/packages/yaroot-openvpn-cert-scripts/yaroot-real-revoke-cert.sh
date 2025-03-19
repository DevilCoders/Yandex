#! /usr/bin/env bash

CERT_DIR="/srv/certs"
CA_CONFIG="$CERT_DIR/ca.config"
CN=$1
LOG_FILE="/var/log/yaroot/yaroot-revoke-cert.log"
LOCK_FILE="$CERT_DIR/revoke.lock"
OPENSSL=`which openssl`

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
    if [[ -z $CN ]]; then
        log_err "arguments too low" && exit 1
    fi
    log_ok
}
check_args

function check_lock() {
    log_info "check lock file..."
    while `/usr/lib/etcd/etcdctl get /revoke.lock 1>/dev/null 2>&1`; do
        log_add "locked from `/usr/lib/etcd/etcdctl get /revoke.lock`"
        sleep 2
    done
    log_done

    log_info "create lock file..."
    #echo $HOSTNAME > $LOCK_FILE
    err_msg=$(/usr/lib/etcd/etcdctl set /revoke.lock $(hostname -f) --ttl 900 2>&1 1>/dev/null)
    if [[ $? != 0 ]]; then
        log_err "can't create lock file: $err_msg" exit 1
    fi
    sleep 2
    log_ok
}
#check_lock

function remove_lock() {
    log_info "removing lock file..."
    err_msg=`/usr/lib/etcd/etcdctl rm /revoke.lock 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't remove lock file: $err_msg"
    fi
    log_ok
}

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

function remove_cert() {
    log_info "revoke cert ${CN}.crt..."
    err_msg=`$OPENSSL ca -config $CA_CONFIG -revoke $CERT_DIR/certs/"$CN".crt 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't revoke ${CN}.crt: $err_msg" && remove_lock && exit 2
    fi
    log_ok
}
remove_cert

function update_crl() {
    log_info "update certificate revocation list..."
    err_msg=`$OPENSSL ca -gencrl -config $CA_CONFIG -out $CERT_DIR/RootCA.crl 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't update certificate revocation list: $err_msg" && remove_lock && exit 3
    fi
    log_ok
}
update_crl

function get_vpn_hosts() {
    log_info "get vpn hosts..."
    eval env=$(cat /etc/yandex/environment.type)
    if [[ $env == "testing" ]]; then
        vpn_hosts=$(curl -s 'https://c.yandex-team.ru/api/groups2hosts/root_vpn-testing' | perl -pe 's/\n/ /')
    elif [[ $env == "production" ]]; then
        vpn_hosts=$(curl -s 'https://c.yandex-team.ru/api/groups2hosts/root_vpn' | perl -pe 's/\n/ /')
    else
        log_err "Environment not detected!" && exit 4
    fi
    log_ok
}
get_vpn_hosts

function kill_vpn() {
    log_info "kill gamer $CN vpn connection..."
    for host in $vpn_hosts; do
        ssh $host "echo \"kill $CN\" | nc 127.0.0.1 1121 1>/dev/null 2>&1"
    done
    log_ok
}
kill_vpn

function chown_dir() {
    chown -R corba:corba $CERT_DIR
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

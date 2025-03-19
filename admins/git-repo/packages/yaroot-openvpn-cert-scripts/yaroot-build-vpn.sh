#! /usr/bin/env bash

CERT_DIR="/srv/certs"
KEY_DIR="$CERT_DIR/vpn-keys"
CA_CONFIG="$CERT_DIR/ca.config"
LOG_FILE="/var/log/yaroot/yaroot-build-vpn.log"
OPENSSL=`which openssl`
CN=$1
EMAIL="nemca@yandex-team.ru"
OPENSSL=`which openssl`

if [[ -z $CN ]]; then
    echo "Usage: $0 vpn-server.name.yandex.net" && exit 1
fi

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

cd $CERT_DIR

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

function check_already_create() {
    log_info "check already created sertificate..."
    egrep "$CN" index.txt 1>/dev/null 2>&1
    if [[ $? == 0 ]]; then
        log_err "certificate to $CN already exist!" && echo "certificate to $CN already exist!" 1>&2 && exit 1
    fi
}
check_already_create

function gen_vpn_key() {
    log_info "create private key and request to a CA to $CN..."
    err_msg=`$OPENSSL req -new -newkey rsa:2048 -nodes -keyout $KEY_DIR/"$CN".key -subj "/C=RU/ST=Moscow/L=Moscow/O=root.yandex.com SelfSignedCA/CN=$CN/emailAddres=$EMAIL" -out $KEY_DIR/"$CN".csr 2>&1 1>/dev/null && chmod 600 $KEY_DIR/"$CN".key 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't create private key and request to a CA to $CN: $err_msg" && exit 2
    fi 
    log_ok
}
gen_vpn_key

function gen_vpn_crt() {
    log_info "signing requests and remove request to $CN..."
    err_msg=`$OPENSSL ca -batch -config $CA_CONFIG -in $KEY_DIR/"$CN".csr -out $KEY_DIR/"$CN".crt 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't create vpn certificat to $CN: $err_msg" && exit 3
    fi 
    log_ok
}
gen_vpn_crt

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

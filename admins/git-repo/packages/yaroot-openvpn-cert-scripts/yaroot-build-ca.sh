#! /usr/bin/env bash

EMAIL="nemca@yandex-team.ru"
CERT_DIR="/srv/certs"
CA_CONFIG="$CERT_DIR/ca.config"
LOG_FILE="/var/log/yaroot/yaroot-build-ca.log"
OPENSSL=`which openssl`
OPENVPN=`which openvpn`

echo -ne "Are you sure? (Y/n) "
read ANSWER
if [ "$ANSWER" != "y" -a "$ANSWER" != "Y" ]; then
    echo "Bye." && exit 1
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
# Remove old certificates, keys and 
function remove_old() {
    log_info "check old files..."
    for FILE in RootCA.key RootCA.crt ca_config RootCA.crl dh2048.pem tls.key; do
        if [ -e $CERT_DIR/$FILE ]; then
            log_add "remove $FILE"
            err_msg=`rm -f $CERT_DIR/$FILE 2>&1 1>/dev/null`
            if [[ $? != 0 ]]; then
                log_err "can't remove $FILE: $err_msg" && exit 1
            fi
        fi
    done
    for DIR in certs keys p12 requests vpn-keys; do
        err_msg=`find $CERT_DIR/$DIR -type f -delete 2>&1 1>/dev/null`
        if [[ $? != 0 ]]; then
            log_err "can't remove files from $DIR: $err_msg" && exit 1  
        fi
    done
    log_done
}
remove_old

function gen_ca_key() {
    log_info "generation CA private key..."
    err_msg=`$OPENSSL genrsa -out $CERT_DIR/RootCA.key 4096 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't generate CA private key: $err_msg" && exit 2
    fi
    log_ok
}
gen_ca_key

function gen_ca_crt() {
    log_info "generation CA certificate..."
    err_msg=`$OPENSSL req -x509 -sha256 -new -key $CERT_DIR/RootCA.key -days 1095 -out $CERT_DIR/RootCA.crt -subj "/C=RU/ST=Moscow/L=Moscow/O=root.yandex.com SelfSignedCA/CN=root.vpn.yandex.com/emailAddress=$EMAIL" 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't generate CA certificate: $err_msg" && exit 3
    fi
    log_ok
}
gen_ca_crt

function build_ca_config() {
    log_info "create ca.config..."
    cat > $CA_CONFIG <<EOF
[ ca ]
default_ca = CA_CLIENT

[ CA_CLIENT ]
dir = $CERT_DIR
certs = \$dir/certs
new_certs_dir = \$dir/certs
database    = \$dir/index.txt
serial      = \$dir/serial

certificate = \$dir/RootCA.crt
private_key = \$dir/RootCA.key

default_days = 365
default_crl_days = 7
default_md = sha256

policy = policy_anything
[ policy_anything ]
countryName     =   optional
stateOrProvinceName = optional
localityName    =   optional
organizationName    =   optional
organizationalUnitName = optional
commonName = supplied
emailAddress = optional
EOF
    if [[ $? != 0 ]]; then
        log_err "can't create ca.config: $err_msg" && exit 4
    fi
    log_ok
}
build_ca_config

function clear_ca_index() {
    log_info "clearing index..."
    for index in index.txt index.txt.attr index.txt.attr.old index.txt.old; do
        /usr/lib/etcd/etcdctl set /$index "" 1>/dev/null 2>&1
    done
    for index in serial serial.old; do
        /usr/lib/etcd/etcdctl set /$index "01" 1>/dev/null 2>&1
    done
    log_ok
}
clear_ca_index

function get_index() {
    log_info "get index files..."
    /usr/lib/etcd/etcdctl set /index.txt "`cat $CERT_DIR/index.txt.temp`" 1>/dev/null 2>&1
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

function gen_ca_crl() {
    log_info "Generation certificate revocation list..."
    err_msg=`$OPENSSL ca -gencrl -config $CA_CONFIG -out $CERT_DIR/RootCA.crl 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't generate certificate revocation list: $err_msg" && exit 5
    fi
    log_ok
}
gen_ca_crl

function gen_tls() {
    log_info "Generation tls-auth key..."
    err_msg=`$OPENVPN --genkey --secret ta.key 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't generate tls-auth key: $err_msg" && exit 6
    fi
    log_ok
}
gen_tls

function gen_dh() {
    log_info "Generation Diffie-Hellman key..."
    err_msg=`$OPENSSL dhparam -out $CERT_DIR/dh2048.pem 2048 2>&1 1>/dev/null`
    if [[ $? != 0 ]]; then
        log_err "can't generate Diffie-Hellman key: $err_msg" && exit 7
    fi
    log_ok
}
gen_dh

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

echo -ne "`now_time` [INFO] congratulations!\n" >> $LOG_FILE

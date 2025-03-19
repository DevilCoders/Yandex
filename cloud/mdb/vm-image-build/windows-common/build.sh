#!/bin/bash

set -x
set -e
PRODUCT=$1
PRODUCT_VERSION="$2"
if [ -z "$PRODUCT_VERSION" ]; then
    PRODUCT_TARGET="$PRODUCT.img"
else
    PRODUCT_TARGET="$PRODUCT-$2.img"
fi

echo "PRODUCT_TARGET is $PRODUCT_TARGET"

if [ -z "$PRODUCT_VERSION" ] && [ $PRODUCT -eq "sqlserver" ]; then
    echo "SQLServersion not specified, using 2016sp2ent"
    SQLSERVER_VERSION="2016sp2ent"
    SQLSERVER_TARGET="sqlserver.img"
fi

PACKER_PROC=

list_descendants () {
    echo "$1"
    local children=$(ps -o pid= --ppid "$1")
    for pid in $children; do
        list_descendants "$pid"
    done
}

kill_packer() {
    if [ "${PACKER_PROC}" != "" ] && kill -0 "${PACKER_PROC}"; then
        kill -9 $(list_descendants "${PACKER_PROC}")
    fi
}

clean_up() {
    [ -n "$SUDO_UID" ] && ls ../$PRODUCT*.log && chown $SUDO_UID ../$PRODUCT*.log || true
    umount /tmp/virtio-win || true
    rm -rf ./packer_cache
    rm -rf ./output
    rm -rf ./floppy
    rm -rf /tmp/virtio-win*
    rm -rf /tmp/packer-log*
}

fail() {
    kill_packer
    clean_up
    echo ""
    echo "FAILED $1"
    exit 1
}

cancel() {
    fail "CTRL-C detected"
}

ssh_cmd() {
    ssh Build@fd01:ffff:ffff:ffff::2 -i ./floppy/id_rsa "$@"
}

trap cancel INT

# clean old data
clean_up

# prepare dirs
mkdir ./floppy
mkdir ./output

# Download and prepare dirvers
s3cmd -c /etc/s3cmd.cfg get s3://mdb-windows/virtio-win-0.1.185_amd64.vfd /tmp/virtio-win-0.1.185_amd64.vfd
mkdir -p /tmp/virtio-win
mount -o loop /tmp/virtio-win-0.1.185_amd64.vfd /tmp/virtio-win
cp -r /tmp/virtio-win/amd64/Win10 ./floppy/Win10
umount /tmp/virtio-win
rm -rf /tmp/virtio-win-0.1.185_amd64.vfd /tmp/virtio-win

# prepare keys
cp -f /etc/dbaas-vm-setup/minion.{pub,pem} ./floppy/
rm -rf ./floppy/id_rsa*
ssh-keygen -N "" -f ./floppy/id_rsa

# version
echo $PRODUCT > ./floppy/PRODUCT
echo $PRODUCT_VERSION > ./floppy/PRODUCT_VERSION

# run build
trap '' SIGTTOU
packer build --color=false packer-$PRODUCT.json &
PACKER_PROC=$!

set +x
set +e

# wait for packer to stop, to error file to be created
while kill -0 "${PACKER_PROC}" 2>/dev/null
do
    # save the logs
    ssh_cmd cat 'C:\Logs\Bootstrap.log' > ../$PRODUCT-bootstrap.tmp.log 2>/dev/null
    ssh_cmd cat 'C:\Logs\Highstate.out.log' > ../$PRODUCT-highstate.out.tmp.log 2>/dev/null
    ssh_cmd cat 'C:\Logs\Highstate.err.log' > ../$PRODUCT-highstate.err.tmp.log 2>/dev/null
    [ -s ../$PRODUCT-bootstrap.tmp.log ] && mv -f ../$PRODUCT-bootstrap.tmp.log ../$PRODUCT-bootstrap.log
    [ -s ../$PRODUCT-highstate.out.tmp.log ] && mv -f ../$PRODUCT-highstate.out.tmp.log ../$PRODUCT-highstate.out.log
    [ -s ../$PRODUCT-highstate.err.tmp.log ] && mv -f ../$PRODUCT-highstate.err.tmp.log ../$PRODUCT-highstate.err.log
    [ -n "$SUDO_UID" ] && ls ../$PRODUCT*.log > /dev/null && chown $SUDO_UID ../$PRODUCT*.log || true
    if ssh_cmd cat 'C:\Bootstrap.error' 2>/dev/null; then
        fail 'Bootstrap script failed'
    fi
    sleep 10
done

# packer stoped by timeout
if [ ! -f ./output/$PRODUCT/packer-$PRODUCT.img ]; then
    fail "image was not created"
fi

mv ./output/$PRODUCT/packer-$PRODUCT.img ../$PRODUCT_TARGET

clean_up

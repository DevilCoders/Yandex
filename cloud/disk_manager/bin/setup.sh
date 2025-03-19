#!/usr/bin/env bash

abspath() {
    path="$1"

    if [ ! -d "$path" ]; then
        echo $path
        return
    fi

    (cd "$1"; pwd)
}

# arguments: $1: build directory
# $2 - $n: files to check
build_dir_mtime() {
    build_dir="$1"
    shift
    build_files=$@

    max_mtime=0
    for f in $build_files
    do
        if [ ! -f "$build_dir/$f" ]; then
            echo 0
            return
        fi

        mtime=`stat -c "%Y" "$build_dir/$f"`
        if [ $mtime -gt $max_mtime ]; then
            max_mtime=$mtime
        fi
    done

    echo $max_mtime
}

# arguments: $1: arcadia directory
# $2 - $n: files to find
find_build_dir() {
    arcadia_dir="$1"
    build_root_candidates=`echo "$arcadia_dir/../ybuild" "$arcadia_dir/../ybuild/*" "$arcadia_dir/../build" "$arcadia_dir/ybuild"`
    shift

    max_mtime=0
    max_build_dir=""

    for build_root in $build_root_candidates
    do
        [ -d "$build_root" ] || continue

        for build_type in "release" "debug"
        do
            build_dir=`abspath "$build_root/$build_type"`
            [ -d "$build_dir" ] || continue

            mtime=`build_dir_mtime "$build_dir" $@`
            if [ $mtime -gt 0 -a $mtime -gt $max_mtime ]; then
                max_mtime=$mtime
                max_build_dir="$build_dir"
            fi
        done
    done

    #if not yet found try to check arcadia dir itself
    if [ -z "$max_build_dir" ]; then
        max_build_dir="$arcadia_dir"
        for f in $@; do
            [ -e "$arcadia_dir/$f" ] || max_build_dir=""
        done
    fi

    echo "$max_build_dir"
}

find_arcadia_dir() {
    readlink -e `dirname "$0"`/../../../
}

BUILD_FILES=" \
    cloud/blockstore/daemon/blockstore-server \
    cloud/compute/snapshot/cmd/yc-snapshot/yc-snapshot \
    cloud/compute/snapshot/cmd/yc-snapshot-populate-database/yc-snapshot-populate-database \
    cloud/disk_manager/cmd/yc-disk-manager/yc-disk-manager \
    cloud/disk_manager/cmd/yc-disk-manager-admin/yc-disk-manager-admin \
    cloud/disk_manager/cmd/yc-disk-manager-init-db/yc-disk-manager-init-db \
    cloud/disk_manager/test/mocks/accessservice/accessservice-mock \
    cloud/disk_manager/test/mocks/metadata/metadata-mock \
    kikimr/driver/kikimr \
    kikimr/tools/cfg/bin/kikimr_configure \
    "

ARCADIA_DIR="`find_arcadia_dir`"
BUILD_DIR="$(find_build_dir "$ARCADIA_DIR" $BUILD_FILES)"

if [ -z "$BUILD_DIR" ]; then
    echo "ERROR: No suitable build directory found"
    exit 2
fi

PERSISTENT_TMP_DIR="$HOME/tmp/disk-manager"
WORKING_DIR="$ARCADIA_DIR/cloud/disk_manager/bin"
OUTPUT_DIRS=" \
    bin \
    data \
    logs \
    "

for dir in $OUTPUT_DIRS; do
    mkdir -p "$PERSISTENT_TMP_DIR/$dir"
    ln -svfT "$PERSISTENT_TMP_DIR/$dir" "$WORKING_DIR/$dir"
done

BIN_DIR="$WORKING_DIR/bin"
DATA_DIR="$WORKING_DIR/data"

# Create symlinks
for file in $BUILD_FILES; do
    ln -svf "$BUILD_DIR/$file" "$BIN_DIR/"
done

generate_cert() {
    local pass="pass123"
    local name="$1"
    local dir="$2"
    openssl genrsa
    openssl genrsa -passout pass:$pass -des3 -out $dir/$name.key 4096
    openssl req -passin pass:$pass -new -key $dir/$name.key -out $dir/$name.csr -subj "/C=RU/L=SaintPetersburg/O=Yandex/OU=Infrastructure/CN=localhost"
    openssl x509 -req -passin pass:$pass -days 365 -in $dir/$name.csr -signkey $dir/$name.key -set_serial 01 -out $dir/$name.crt -extfile <(printf "subjectAltName=DNS:$(hostname --fqdn),DNS:localhost")
    openssl rsa -passin pass:$pass -in $dir/$name.key -out $dir/$name.key
    rm $dir/$name.csr
}

generate_cert "server" "$DATA_DIR"

touch "$WORKING_DIR/restarts.txt"

mkdir $DATA_DIR/yc-disk-manager-admin
echo "TestIAMToken" > $DATA_DIR/yc-disk-manager-admin/iam-token

#!/usr/bin/env bash

CLUSTER=${CLUSTER:-local}

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`
DATA_DIR=$BIN_DIR/data

set -e
bash $BIN_DIR/dynamic/init_storage.bash
bash $BIN_DIR/dynamic/init_compute.bash
bash $BIN_DIR/dynamic/init_root_storage.bash
bash $BIN_DIR/dynamic/init_databases.bash
bash $BIN_DIR/dynamic/init_cms.bash

GRPC_PORT=${GRPC_PORT:-9001}

# allow custom configs

ALLOW_NAMED_CONFIGS_REQ="
ConfigsConfig {
    UsageScopeRestrictions {
        AllowedTenantUsageScopeKinds: 100
        AllowedHostUsageScopeKinds:   100
        AllowedNodeTypeUsageScopeKinds: 100
    }
}
"
$BIN_DIR/kikimr -s grpc://localhost:$GRPC_PORT admin console config set \
    --merge "$ALLOW_NAMED_CONFIGS_REQ"

$BIN_DIR/kikimr -s grpc://localhost:$GRPC_PORT db schema user-attribute \
    set /Root/NBS __volume_space_limit_ssd_nonrepl=1

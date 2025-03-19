#!/usr/bin/env bash

find_bin_dir() {
    readlink -e `dirname $0`
}

BIN_DIR=`find_bin_dir`

GRPC_PORT=${GRPC_PORT:-9001}

# update space limit

$BIN_DIR/kikimr -s grpc://localhost:$GRPC_PORT db schema user-attribute \
    set /Root/NBS __volume_space_limit_ssd_nonrepl=$(( 5 * 1024**3 + 512 * 1024**2 ))

# setup local agent

truncate -s   1G "$BIN_DIR/data/fox14954f120312e0404e9a02cc068ac.bin"
truncate -s   1G "$BIN_DIR/data/fox2facdcd9a018a89bb228786cb17e0.bin"
truncate -s 256M "$BIN_DIR/data/fox3locald9a018a89bb228786cb17eb.bin"
truncate -s 256M "$BIN_DIR/data/fox4global9a018a89bb228786cb17eb.bin"

cat > $BIN_DIR/nbs/nbs-disk-agent-1.txt <<EOF
AgentId: "`hostname`"
Enabled: true
DedicatedDiskAgent: true
Backend: DISK_AGENT_BACKEND_AIO

FileDevices: {
    Path: "$BIN_DIR/data/fox14954f120312e0404e9a02cc068ac.bin"
    DeviceId: "fox14954f120312e0404e9a02cc068ac"
    BlockSize: 4096
}

FileDevices: {
    Path: "$BIN_DIR/data/fox2facdcd9a018a89bb228786cb17e0.bin"
    DeviceId: "fox2facdcd9a018a89bb228786cb17e0"
    BlockSize: 4096
}

FileDevices: {
    Path: "$BIN_DIR/data/fox3locald9a018a89bb228786cb17eb.bin"
    DeviceId: "fox3locald9a018a89bb228786cb17eb"
    BlockSize: 4096
    PoolName: "local-ssd"
}

FileDevices: {
    Path: "$BIN_DIR/data/fox4global9a018a89bb228786cb17eb.bin"
    DeviceId: "fox4global9a018a89bb228786cb17eb"
    BlockSize: 4096
    PoolName: "local-ssd"
}

EOF

cat > $BIN_DIR/nbs/nbs-location-1.txt <<EOF
DataCenter: "localhost"
Rack: "rack-1"
EOF

# setup remote agent

truncate -s   1G "$BIN_DIR/data/dana15604954f120312e0404e9a02cc0.bin"
truncate -s   1G "$BIN_DIR/data/dana2652facdcd9a018a89bb228786cb.bin"
truncate -s  32M "$BIN_DIR/data/dana3local12acdcd9a018a89bb22878.bin"
truncate -s  32M "$BIN_DIR/data/dana4global9a018a89bb228786cb17e.bin"

cat > $BIN_DIR/nbs/nbs-disk-agent-2.txt <<EOF
AgentId: "remote.cloud.yandex.net"
Enabled: true
DedicatedDiskAgent: true
Backend: DISK_AGENT_BACKEND_AIO

FileDevices: {
    Path: "$BIN_DIR/data/dana15604954f120312e0404e9a02cc0.bin"
    DeviceId: "dana15604954f120312e0404e9a02cc0"
    BlockSize: 4096
}

FileDevices: {
    Path: "$BIN_DIR/data/dana2652facdcd9a018a89bb228786cb.bin"
    DeviceId: "dana2652facdcd9a018a89bb228786cb"
    BlockSize: 4096
}

FileDevices: {
    Path: "$BIN_DIR/data/dana3local12acdcd9a018a89bb22878.bin"
    DeviceId: "dana3local12acdcd9a018a89bb22878"
    BlockSize: 4096
    PoolName: "small"
}

FileDevices: {
    Path: "$BIN_DIR/data/dana4global9a018a89bb228786cb17e.bin"
    DeviceId: "dana4global9a018a89bb228786cb17e"
    BlockSize: 4096
    PoolName: "small"
}
EOF

cat > $BIN_DIR/nbs/nbs-location-2.txt <<EOF
DataCenter: "localhost"
Rack: "rack-2"
EOF

# setup Disk Registry config

cat > $BIN_DIR/nbs/nbs-disk-registry.txt <<EOF
KnownAgents {
    AgentId: "`hostname`"
    Devices: "fox14954f120312e0404e9a02cc068ac"
    Devices: "fox2facdcd9a018a89bb228786cb17e0"
    Devices: "fox3locald9a018a89bb228786cb17eb"
    Devices: "fox4global9a018a89bb228786cb17eb"
}

KnownAgents {
    AgentId: "remote.cloud.yandex.net"
    Devices: "dana15604954f120312e0404e9a02cc0"
    Devices: "dana2652facdcd9a018a89bb228786cb"
    Devices: "dana3local12acdcd9a018a89bb22878"
    Devices: "dana4global9a018a89bb228786cb17e"
}

KnownDevicePools {
    Name: "local-ssd"
    AllocationUnit: 268435456
    Kind: DEVICE_POOL_KIND_LOCAL
}

KnownDevicePools {
    Name: "small"
    AllocationUnit: 33554432
    Kind: DEVICE_POOL_KIND_GLOBAL
}

EOF

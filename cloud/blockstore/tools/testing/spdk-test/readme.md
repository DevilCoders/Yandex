# run perf againts spdk-test NVMe-oF target
```bash
sudo ./spdk-test \
    --test Target \
    --memory-device disk1 \
    --memory-device-block-size 4096 \
    --memory-device-blocks-count 1024 \
    --nvme-target-transport 'trtype=TCP adrfam=IPv4 traddr=127.0.0.1 trsvcid=4420' \
    --nvme-target-nqn 'nqn.2018-09.io.spdk:cnode1'

sudo nvme discover --transport=tcp --traddr=127.0.0.1 --trsvcid=4420

sudo ./perf \
    -r 'trtype=TCP adrfam=IPv4 traddr=127.0.0.1 trsvcid=4420 subnqn:nqn.2018-09.io.spdk:cnode1' \
    -q 10 -o 4096 -w randwrite -t 120 \
    -c 2
```

# run spdk-test against nvmf_tgt target
```bash
sudo ./nvmf_tgt

sudo ./scripts/rpc.py bdev_malloc_create -b disk1 1024 4096
sudo ./scripts/rpc.py nvmf_create_transport -t TCP
sudo ./scripts/rpc.py nvmf_create_subsystem nqn.2018-09.io.spdk:cnode1 -s SPDK001 -a -m 8
sudo ./scripts/rpc.py nvmf_subsystem_add_ns nqn.2018-09.io.spdk:cnode1 disk1
sudo ./scripts/rpc.py nvmf_subsystem_add_listener nqn.2018-09.io.spdk:cnode1 -t tcp -f ipv4 -s 4420 -a 127.0.0.1

sudo nvme discover --transport=tcp --traddr=127.0.0.1 --trsvcid=4420

sudo ./spdk-test \
    --test Initiator \
    --nvme-device-transport 'trtype=TCP adrfam=IPv4 traddr=127.0.0.1 trsvcid=4420' \
    --nvme-device-nqn 'nqn.2018-09.io.spdk:cnode1'
```

# run spdk-test against iscsi_tgt target
```bash
sudo ./iscsi_tgt

sudo ./scripts/rpc.py bdev_malloc_create -b disk1 1024 4096
sudo ./scripts/rpc.py iscsi_create_portal_group 1 127.0.0.1:3260
sudo ./scripts/rpc.py iscsi_create_initiator_group 1 ANY ANY
sudo ./scripts/rpc.py iscsi_create_target_node scsi1 "Data Disk1" "disk1:0" 1:1 64 -d

sudo iscsiadm -m discovery -t sendtargets -p 127.0.0.1:3260

sudo ./spdk-test \
    --test Initiator \
    --scsi-device-url 'iscsi://127.0.0.1:3260/iqn.2016-06.io.spdk:scsi1/0'
```

# run spdk-test against spdk-test iSCSI target
```bash
sudo ./spdk-test \
    --test Target \
    --memory-device disk1 \
    --memory-device-block-size 4096 \
    --memory-device-blocks-count 1024 \
    --scsi-target scsi1 \
    --scsi-target-port 3260

sudo iscsiadm -m discovery -t sendtargets -p 127.0.0.1:3260

sudo ./spdk-test \
    --test Initiator \
    --scsi-device-url 'iscsi://127.0.0.1:3260/iqn.2016-06.io.spdk:scsi1/0'
```

# run spdk-test against local device
```bash
sudo ./spdk-test \
    --test Initiator \
    --memory-device disk1 \
    --memory-device-block-size 4096 \
    --memory-device-blocks-count 1024
```

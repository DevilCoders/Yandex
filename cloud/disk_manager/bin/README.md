# Local debugging setup

Build
```bash
ya make ../buildall
```

Setup
```bash
./setup.sh
```

Configure and run IAM mocks
```bash
./accessservice-mock.sh >logs/accessservice-mock.log 2>&1 &
./metadata-mock.sh >logs/metadata-mock.log 2>&1 &
```

Configure and run kikimr
```bash
./kikimr-configure.sh
./kikimr-format.sh
./kikimr-server.sh >logs/kikimr-server.log 2>&1 &
./kikimr-init.sh
```

Start NBS
```bash
./blockstore-server.sh >logs/blockstore-server.log 2>&1 &
```

Init & Start Snapshot Service
```bash
./yc-snapshot-init.sh
./yc-snapshot.sh >logs/yc-snapshot.log 2>&1 &
```
NOTE: to use image transfer you need `qemu-nbd` & `qemu-img` in your `PATH`,
and `nbd` kernel module loaded.
Also you need docker installed and your user (sudoer) should be added to the docker group:
```
apt-get install docker.io
sudo usermod -aG docker $username
```

Start Disk Manager Service
```bash
./yc-disk-manager-init.sh
./yc-disk-manager.sh >logs/yc-disk-manager.log 2>&1 &
```

For interaction with Disk Manager use
```bash
./yc-disk-manager-admin.sh <YOUR ARGS>
```

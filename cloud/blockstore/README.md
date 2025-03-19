# Setup for local debugging

\# Build from sources
```bash
cd arcadia/cloud/blockstore/bin
ya make ../buildall --checkout
```

\# Prepare BIN directory
```bash
./setup.sh
```

\# Generate KiKiMR configuration
```bash
./kikimr-configure.sh
```

\# Start KiKiMR storage
```bash
./kikimr-format.sh
./kikimr-server.sh >logs/kikimr-server.log 2>&1 &
# wait a bit while server is starting
./kikimr-init.sh
```
If you have trouble with this step, and you encrypted home directory using ```ecryptfs``` 
you have to create local disks outside home(encrypted) directory, e.g. ```/var/tmp```. 
Change it in ```setup.sh``` from ```"$HOME/tmp/nbs"``` to your own and do all steps from setup again

\# Start NBS service
```bash
./blockstore-server.sh >logs/blockstore-server.log 2>&1 &
```
or
```bash
./configure-nonrepl.sh
./blockstore-server.sh >logs/blockstore-server.log 2>&1 &
./blockstore-client.sh UpdateDiskRegistryConfig --verbose error --input nbs/nbs-disk-registry.txt --proto
./blockstore-client.sh ExecuteAction --action allowdiskallocation --verbose error --input-bytes '{"Allow":true}'

MON_PORT=8769 IC_PORT=29012 ./blockstore-disk-agent.sh --location-file nbs/nbs-location-1.txt --disk-agent-file nbs/nbs-disk-agent-1.txt >logs/mulder.log 2>&1 &
MON_PORT=8969 IC_PORT=29112 ./blockstore-disk-agent.sh --location-file nbs/nbs-location-2.txt --disk-agent-file nbs/nbs-disk-agent-2.txt >logs/scully.log 2>&1 &
```

\# Create NBS volume
```bash
./blockstore-client.sh CreateVolume --disk-id=vol0 --verbose error --blocks-count=100000000
```
or 
```bash
./blockstore-client.sh CreateVolume --disk-id=nrd0 --verbose error --storage-media-kind nonreplicated --blocks-count=524288
```
or 
```bash
./blockstore-client.sh CreateVolume --disk-id=local0 --verbose error --storage-media-kind ssd_local --blocks-count=65536 --agent-id `hostname`
```

\# Start VM on NBS volume
```bash
./qemu-vm.sh >logs/qemu.log 2>&1 &
```

\# Connect to VM console
```bash
vncviewer 127.0.0.1
```

#!/bin/bash -e

set -v

mkdir -p "/mnt/corefs"
mount "/dev/vda6" "/mnt/corefs"

mkdir /mnt/corefs/base

cat > /mnt/corefs/grub.cfg <<EOF
set linux_append="coreos.config.url=http://169.254.169.254/latest/user-data net.ifnames=0 coreos.oem.id=ec2"
set linux_console="console=ttyS0,115200n8"
serial com0 --speed=115200 --word=8 --parity=no
terminal_input serial_com0
terminal_output serial_com0
EOF

cat > /mnt/corefs/base/base.ign <<EOF
{
  "ignition": {
    "version": "2.1.0"
  },
  "systemd": {
    "units": [
      {
        "name": "coreos-metadata-sshkeys@.service",
        "enabled": true
      }
    ]
  }
}
EOF

cat > /mnt/corefs/base/default.ign <<EOF
{
  "ignition": {
    "version": "2.1.0"
  },
  "systemd": {
    "units": [
      {
        "name": "user-configdrive.service",
        "mask": true
      },
      {
        "name": "user-configvirtfs.service",
        "mask": true
      },
      {
        "name": "oem-cloudinit.service",
        "enabled": true,
        "contents": "[Unit]\nDescription=Cloudinit from platform metadata\n\n[Service]\nType=oneshot\nExecStart=/usr/bin/coreos-cloudinit --oem=ec2-compat\n\n[Install]\nWantedBy=multi-user.target\n"
      }
    ]
  }
}
EOF

umount "/mnt/corefs"
echo "Installation completed"

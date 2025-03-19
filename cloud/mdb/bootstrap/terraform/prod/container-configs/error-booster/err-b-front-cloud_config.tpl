#cloud-config
serial-port-enable: 1
bootcmd:
    - dhclient -6 -v eth0
runcmd:
    - [ sudo, chmod, "666", "/dev/ttyS1"]
ssh_pwauth: no
users:
    - default
    -   name: chapson
        sudo: ALL=(ALL) NOPASSWD:ALL
        shell: /bin/bash
        ssh_authorized_keys:
            - ${chapson-key}
    -   name: khattu
        sudo: ALL=(ALL) NOPASSWD:ALL
        shell: /bin/bash
        ssh_authorized_keys:
            - ${khattu-key}
fqdn: ${host}
hostname: ${host}

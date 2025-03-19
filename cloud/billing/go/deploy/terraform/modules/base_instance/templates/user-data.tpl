#cloud-config
ssh_pwauth: no
users:
  - name: debug
    sudo: ALL=(ALL) NOPASSWD:ALL
    shell: /bin/bash
    passwd: $6$rounds=4096$ej5Z5As2j5Pb91i$mw93Tbx8I6zOIW1S5DY.1N0yRjvjuJejQUomQqixe/ZpzljBQY6bf3qn5N2XssKP2jBHOv7H7yD9QRN0pmZPR/
    lock_passwd: false
write_files:
  - content: |
      ${ netplan_config }
    path: /etc/netplan/99-eth-override.yaml
runcmd:
  - [netplan, apply]

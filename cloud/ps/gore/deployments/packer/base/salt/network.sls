new template for cloud-init to form /etc/hosts:
  file.managed:
    - name: /etc/cloud/templates/hosts.debian.tmpl
    - source: salt://network/hosts.debian.tmpl

remove old network cfg files:
  file.directory:
    - name: /etc/netplan/
    - clean: True

set static interfaces file:
  file.managed:
    - name: /etc/netplan/10-netcfg.yaml
    - source: salt://network/netcfg.yaml

sysctl settings for network:
  file.managed:
    - name: /etc/sysctl.d/30-base-network.conf
    - source: salt://network/net-sysctl.conf

dhcpv6 client settings:
  file.managed:
    - name: /etc/dhcp/dhclient6.conf
    - source: salt://network/dhclient6.conf

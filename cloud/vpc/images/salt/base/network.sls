new template for cloud-init to form /etc/hosts:
  file.managed:
    - name: /etc/cloud/templates/hosts.debian.tmpl
    - source: salt://{{ slspath }}/network/hosts.debian.tmpl

remove old network cfg files:
  file.directory:
    - name: /etc/netplan/
    - clean: True

set static interfaces file:
  file.managed:
    - name: /etc/netplan/10-netcfg.yaml
    - source: salt://{{ slspath }}/network/netcfg.yaml

sysctl settings for network:
  file.managed:
    - name: /etc/sysctl.d/30-base-network.conf
    - source: salt://{{ slspath }}/network/net-sysctl.conf

dhcpv6 client settings:
  file.managed:
    - name: /etc/dhcp/dhclient6.conf
    - source: salt://{{ slspath }}/network/dhclient6.conf

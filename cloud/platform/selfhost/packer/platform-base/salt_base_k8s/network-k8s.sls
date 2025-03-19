new template for cloud-init to form /etc/hosts:
  file.managed:
    - name: /etc/cloud/templates/hosts.debian.tmpl
    - source: salt://network/hosts.debian.tmpl
    - template:

remove old network cfg files:
  file.directory:
    - name: /etc/network/interfaces.d/
    - clean: True

set static interfaces file:
  file.managed:
    - name: /etc/network/interfaces
    - source: salt://network/interfaces-two
    - template:

sysctl settings for network:
  file.managed:
    - name: /etc/sysctl.d/30-network.conf
    - source: salt://network/net-sysctl.conf
    - template:

dhcpv6 client settings:
  file.managed:
    - name: /etc/dhcp/dhclient6.conf
    - source: salt://network/dhclient6.conf
    - template:

Add route table for eth0 interface:
  file.blockreplace:
  - name: /etc/iproute2/rt_tables
  - contents: '10 eth0'
  - append_if_not_found: True

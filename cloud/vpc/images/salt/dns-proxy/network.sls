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
    - name: /etc/sysctl.d/31-dns-proxy-network.conf
    - source: salt://{{ slspath }}/network/net-sysctl.conf

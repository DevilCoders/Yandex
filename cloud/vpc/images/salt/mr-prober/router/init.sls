set static interfaces file:
  file.managed:
    - name: /etc/netplan/10-netcfg.yaml
    - source: salt://{{ slspath }}/files/netcfg.yaml

enable IP forwarding in sysctl:
  file.managed:
    - name: /etc/sysctl.d/99-sysctl.conf
    - source: salt://{{ slspath }}/files/99-sysctl.conf

set static interfaces file:
  file.managed:
    - name: /etc/netplan/10-netcfg.yaml
    - source: salt://{{ slspath }}/files/netcfg.yaml

nginx:
  pkg:
    - installed
  service.running:
    - watch:
      - pkg: nginx

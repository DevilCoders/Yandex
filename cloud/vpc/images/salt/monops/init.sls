# Disable docker0 bridge as it clashes with 172.16.0.0/17 IPv4 SAS network
override-/etc/docker/daemon.json:
  file.managed:
    - name: /etc/docker/daemon.json
    - source: salt://{{ slspath }}/files/docker/daemon.json
  require:
    - file: /etc/docker/daemon.json

monops:
  file.managed:
    - name: /etc/systemd/system/monops.service
    - source: salt://{{ slspath }}/files/monops.service
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: monops
  service.enabled:
    - watch:
      - module: monops

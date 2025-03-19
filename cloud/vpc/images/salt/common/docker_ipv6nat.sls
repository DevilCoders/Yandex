docker-ipv6nat:
  file.managed:
    - name: /etc/systemd/system/docker-ipv6nat.service
    - source: salt://{{ slspath }}/files/docker-ipv6nat.service
  module.run:
    - name: service.systemctl_reload
    - onchanges:
      - file: docker-ipv6nat
  service.enabled:
    - watch:
      - module: docker-ipv6nat

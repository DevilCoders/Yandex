tinyproxy_pkg:
  pkg.installed:
    - name: tinyproxy

tinyproxy_config:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/tinyproxy.conf
    - name: /etc/tinyproxy.conf

tinyproxy_service:
  service.running:
    - name: tinyproxy
    - enable: true
    - reload: true
    - sig: '/usr/sbin/tinyproxy'
    - watch:
      - tinyproxy_config

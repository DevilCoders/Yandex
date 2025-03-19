trapdoor-config:
  file.managed:
    - name: /etc/rsyslog.d/40-yandex-trapdoor.conf
    - source: salt://{{ slspath }}/files/40-yandex-trapdoor.conf

rsyslog:
  service.running:
    - enable: True
    - watch:
      - file: trapdoor-config

/lib/systemd/system/watchdog.service:
  file.managed:
    - source: salt://{{ slspath }}/watchdog.service
    - user: root
    - group: root
    - mode: 644

watchdog.service:
  service.running:
    - enable: True
    - reload: True
    - watch:
      - file: /lib/systemd/system/watchdog.service

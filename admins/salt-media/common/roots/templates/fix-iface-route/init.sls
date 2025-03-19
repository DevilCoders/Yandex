yandex-media-check-route:
  pkg.installed

/etc/cron.d/fix-iface-route:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - source: salt://{{slspath}}/files/fix-iface-route

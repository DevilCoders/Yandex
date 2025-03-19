config_coredump_monitor:
  yafile.managed:
    - name: /etc/yandex/coredump_monitor.conf
    - source: salt://{{ slspath }}/files/etc/yandex/coredump_monitor.conf

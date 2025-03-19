/etc/monitoring/watchdog.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: |
        # MANAGED BY SALT
        # https://a.yandex-team.ru/arc/trunk/arcadia/admins/salt-media/media/roots/dom0-tokk/watchdog.sls
        mysqld => { cpu_warn => 700, cpu_crit => 1000 }

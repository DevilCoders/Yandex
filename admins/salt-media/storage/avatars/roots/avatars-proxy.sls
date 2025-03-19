include:
  - templates.parsers
  - templates.iostat
  - templates.ipvs_tun
  - templates.certificates
  - units.nginx
  - units.walle_juggler_checks

/etc/cron.d/postfix_watchdog:
  file.managed:
    - source: salt://files/avatars-proxy/etc/cron.d/postfix_watchdog
    - user: root
    - group: root
    - mode: 644

/etc/monitoring/elliptics-queue.conf:
  file.managed:
    - source: salt://files/avatars-proxy/etc/monitoring/elliptics-queue.conf
    - user: root
    - group: root
    - mode: 644

include:
  - templates.push-server.server


/etc/monitoring/watchdog.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: /push-server-lite/ warning

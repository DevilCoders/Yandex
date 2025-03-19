salt-minion:
  service.dead:
    - enable: False


/etc/init/salt-minion.override:
  file.managed:
    - contents:
      - "manual"
    - user: root
    - group: root
    - mode: 644

/etc/nginx/conf.d/01-mds-access-log-tskv.conf:
  file.managed:
    - source: salt://files/nginx-common/etc/nginx/conf.d/01-mds-access-log-tskv.conf
    - user: root
    - group: root
    - mode: 644

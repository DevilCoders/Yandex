/etc/monrun/conf.d/nginx_80.conf:
  file.managed:
    - source: salt://files/etc/monrun/conf.d/nginx_80.conf
    - user: root
    - group: root
    - mode: 644

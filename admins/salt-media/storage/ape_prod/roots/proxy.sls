/etc/sysctl.d/:
  file.recurse:
    - source: salt://proxy/etc/sysctl.d/
/etc/nginx/ssl/storagecert.pem:
  file.managed:
    - source: "salt://certs/ape-proxy/storagecert.pem"
    - makedirs: True
    - mode: 600
    - user: root
    - group: root

/etc/nginx/ssl/storagekey.pem:
  file.managed:
    - source: "salt://certs/ape-proxy/storagekey.pem"
    - makedirs: True
    - mode: 600
    - user: root
    - group: root

/etc/nginx/conf.d/01-accesslog.conf:
  file.managed:
    - source: salt://proxy/etc/nginx/conf.d/01-accesslog.conf

/etc/yandex/loggiver/loggiver.pattern:
  file.managed:
    - source: "salt://proxy/etc/yandex/loggiver/loggiver.pattern"

/etc/logrotate.d/nginx-tskv:
  file.managed:
    - source: "salt://proxy/etc/logrotate.d/nginx-tskv"

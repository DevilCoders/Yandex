/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://proxies/nginx/sites-enabled

nginx:
  service:
    - running
    - enable: True
    - reload: True
    - watch:
      - file: /etc/nginx/sites-enabled

/etc/nginx/ssl/storagecert.pem:
  file.managed:
    - source: "salt://certs/ape-test-proxy/storagecert.pem"
    - makedirs: True
    - mode: 600
    - user: root
    - group: root

/etc/nginx/ssl/storagekey.pem:
  file.managed:
    - source: "salt://certs/ape-test-proxy/storagekey.pem"
    - makedirs: True
    - mode: 600
    - user: root
    - group: root

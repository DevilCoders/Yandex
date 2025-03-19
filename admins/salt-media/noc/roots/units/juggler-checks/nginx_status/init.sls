/etc/monrun/salt_nginx_status/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/MANIFEST.json
    - makedirs: True
    - mode: 755

/etc/monrun/salt_nginx_status/nginx_status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/nginx_status.sh
    - makedirs: True
    - mode: 755


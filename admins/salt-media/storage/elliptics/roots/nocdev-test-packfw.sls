include:
  - units.nginx_conf
  - templates.certificates

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/nocdev-test-packfw/etc/nginx/sites-enabled/

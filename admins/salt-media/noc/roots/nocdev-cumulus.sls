include:
  - units.nginx_conf
  - units.juggler-checks.common

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://files/nocdev-cumulus/etc/nginx/sites-enabled/

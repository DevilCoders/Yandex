include:
  - units.netconfiguration
  - units.iface-ip-conf

/etc/default:
  file.recurse:
    - source: salt://registry/etc/default/

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://registry/etc/nginx/sites-enabled/

/etc/systemd/system/multi-user.target.wants/docker.service:
  file.managed:
    - source: salt://registry/etc/systemd/system/multi-user.target.wants/docker.service

/etc/docker/daemon.json:
  file.managed:
    - source: salt://registry/etc/docker/daemon.json

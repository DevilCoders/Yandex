include:
  - units.netconfiguration
  - units.iface-ip-conf

/etc/monrun/conf.d:
  file.recurse:
    - source: salt://storage/etc/monrun/conf.d

/etc/distributed-flock-media.json:
  file.managed:
    - source: salt://storage/etc/distributed-flock-media.json

/etc/elliptics:
  file.recurse:
    - source: salt://storage/etc/elliptics

/etc/cron.d:
  file.recurse:
    - source: salt://storage/etc/cron.d

/usr/local/bin:
  file.recurse:
    - source: salt://storage/usr/local/bin


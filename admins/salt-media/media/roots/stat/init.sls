zk-flock:
  pkg.installed

/etc/distributed-flock.json:
  file.managed:
    - source: salt://stat/zk-flock/distributed-flock.json
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/etc/juggler_flapper_media.conf:
  file.managed:
    - source: salt://stat/juggler-flapper/juggler_flapper_media.conf
    - user: root
    - group: root
    - mode: 644
    - makedirs: True

/usr/bin/juggler_media_flaps.sh:
  file.managed:
    - source: salt://stat/juggler-flapper/juggler_media_flaps.sh
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

zk-flock juggler_flapper /usr/bin/juggler_media_flaps.sh:
  cron.present:
    - identifier: juggler_flaps
    - user: root
    - minute: 0
    - hour: 4
    - dayweek: 1

salt-call state.highstate > /var/log/highstate.log 2>&1:
  cron.present:
    - identifier: highstate
    - user: root
    - minute: '*/5'

/etc/syslog-ng/conf.d/netconsole:
  file.managed:
    - source: salt://stat/syslog-ng/netconsole.conf
    - makedirs: True
  service.running:
    - name: syslog-ng
    - watch:
      - file: /etc/syslog-ng/conf.d/netconsole

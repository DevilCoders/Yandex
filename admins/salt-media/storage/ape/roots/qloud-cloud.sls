/etc/cocaine/orca.yaml:
  file.managed:
    - source: salt://qloud-cloud/etc/cocaine/orca.yaml
    - makedirs: True

/usr/local/bin:
  file.recurse:
    - source: salt://qloud-cloud/usr/local/bin/
    - file_mode: '0755'

create list of zk-dc nodes from conductor group:
  cmd.script:
    - source: salt://qloud-cloud/scripts/zk-dc-get.sh

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://qloud-cloud/etc/cocaine/cocaine.conf
    - makedirs: True

/etc/cocaine/auth.keys:
  file.managed:
    - source: salt://qloud-cloud/etc/cocaine/auth.keys

/usr/local/sbin/ubic-flaps.sh:
  file.managed:
    - source: salt://qloud-cloud/usr/local/sbin/runit-flap.sh
    - mode: 755

/home/monitor/juggler/etc/client.conf:
  file.managed:
    - source: salt://qloud-cloud/home/monitor/juggler/etc/client.conf
    - mode: 644
    - makedirs: True

/etc/juggler/client.conf:
  file.managed:
    - source: salt://qloud-cloud/home/monitor/juggler/etc/client.conf
    - mode: 644
    - makedirs: True

/etc/monrun/conf.d/virtual-meta.conf:
  file.managed:
    - source: salt://qloud-cloud/etc/monrun/conf.d/virtual-meta.conf
    - mode: 644
    - makedirs: True

/etc/yabs-graphite-client/graphite-client.cfg:
  file.managed:
    - source: salt://qloud-cloud/etc/yabs-graphite-client/graphite-client.cfg
    - makedirs: True
/usr/local/yabs-graphite-client/graphite-sender.py:
  file.managed:
    - source: salt://qloud-cloud/usr/local/yabs-graphite-client/graphite-sender.py
    - makedirs: True
    - mode: 755

/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py:
  file.managed:
    - source: salt://common-files/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py
    - mode: 755

/etc/runit/1:
  file.managed:
    - source: salt://qloud-cloud/etc/runit/1
    - mode: 755
    - makedirs: True

/etc/runit/2:
  file.managed:
    - source: salt://qloud-cloud/etc/runit/2
    - mode: 755
    - makedirs: True

/etc/runit/3:
  file.managed:
    - source: salt://qloud-cloud/etc/runit/3
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-runtime/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-runtime/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-runtime/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-runtime/log/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-orca/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-orca/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-orca/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-orca/log/run
    - mode: 755
    - makedirs: True

/etc/sv/skynet/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/skynet/run
    - mode: 755
    - makedirs: True

/etc/sv/skynet/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/skynet/log/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-restart-broken/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-restart-broken/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-restart-broken/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-restart-broken/log/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-warmup/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-warmup/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-warmup/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-warmup/log/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-data/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-data/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-data/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-data/log/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-isolate-daemon/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-isolate-daemon/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-isolate-daemon/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-isolate-daemon/log/run
    - mode: 755
    - makedirs: True

/etc/sv/statbox-push-client/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/statbox-push-client/run
    - mode: 755
    - makedirs: True

/etc/sv/statbox-push-client/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/statbox-push-client/log/run
    - mode: 755
    - makedirs: True

/etc/sv/loggiver/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/loggiver/run
    - mode: 755
    - makedirs: True

/etc/sv/loggiver/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/loggiver/log/run
    - mode: 755
    - makedirs: True

/etc/sv/nginx/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/nginx/run
    - mode: 755
    - makedirs: True

/etc/sv/mecoll/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/mecoll/run
    - mode: 755
    - makedirs: True

/etc/sv/nginx/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/nginx/log/run
    - mode: 755
    - makedirs: True

/etc/sv/mecoll/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/mecoll/log/run
    - mode: 755
    - makedirs: True

/etc/sv/graphite-client/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/graphite-client/run
    - mode: 755
    - makedirs: True

/etc/sv/graphite-client/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/graphite-client/log/run
    - mode: 755
    - makedirs: True

/etc/sv/cocaine-refresh-group/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/cocaine-refresh-group/run
    - mode: 755
    - makedirs: True

/etc/sv/juggler/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/juggler/log/run
    - mode: 755
    - makedirs: True

/etc/sv/juggler/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/juggler/run
    - mode: 755
    - makedirs: True

/etc/sv/yasmagent/log/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/yasmagent/log/run
    - mode: 755
    - makedirs: True

/etc/sv/yasmagent/run:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/yasmagent/run
    - mode: 755
    - makedirs: True

/etc/sv/yasmagent/finish:
  file.managed:
    - source: salt://qloud-cloud/etc/sv/yasmagent/finish
    - mode: 755
    - makedirs: True


/var/log/runit/cocaine-runtime/config:
  file.managed:
    - source: salt://qloud-cloud/var/log/runit/cocaine-runtime/config
    - mode: 755
    - makedirs: True

create finish files for all services in runit:
  cmd.script:
    - source: salt://qloud-cloud/scripts/finish.sh

/etc/service:
  file.directory
/etc/service/cocaine-isolate-daemon:
   file.symlink:
     - target: /etc/sv/cocaine-isolate-daemon
/etc/service/statbox-push-client:
   file.symlink:
     - target: /etc/sv/statbox-push-client
/etc/service/loggiver:
   file.symlink:
     - target: /etc/sv/loggiver
/etc/service/nginx:
   file.symlink:
     - target: /etc/sv/nginx
/etc/service/mecoll:
   file.symlink:
     - target: /etc/sv/mecoll
/etc/service/graphite-client:
   file.symlink:
     - target: /etc/sv/graphite-client
/etc/service/cocaine-runtime:
   file.symlink:
     - target: /etc/sv/cocaine-runtime
/etc/service/cocaine-orca:
   file.symlink:
     - target: /etc/sv/cocaine-orca
/etc/service/cocaine-restart-broken:
   file.symlink:
     - target: /etc/sv/cocaine-restart-broken
/etc/service/cocaine-warmup:
   file.symlink:
     - target: /etc/sv/cocaine-warmup
/etc/service/cocaine-data:
   file.symlink:
     - target: /etc/sv/cocaine-data
/etc/service/cocaine-refresh-group:
   file.symlink:
     - target: /etc/sv/cocaine-refresh-group
/etc/service/skynet:
   file.symlink:
     - target: /etc/sv/skynet
/etc/service/yasmagent:
   file.symlink:
     - target: /etc/sv/yasmagent
/etc/service/juggler:
   file.symlink:
     - target: /etc/sv/juggler


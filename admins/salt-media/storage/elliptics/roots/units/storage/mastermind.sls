syslog-ng-service:
  service.running:
    - name: syslog-ng

mastermind-jobs:
  file.managed:
    - name: /etc/logrotate.d/mastermind-jobs
    - user: root
    - source: salt://files/storage/mastermind-jobs
    - group: root
    - mode: 644

/etc/syslog-ng/conf-enabled/20-mastermind.conf:
  file.managed:
    - source: salt://files/storage/20-mastermind.conf
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - watch_in:
      - service: syslog-ng

mm_libmastermind_cache:
  monrun.present:
    - command: "timetail -t tskv -n 11100 /var/log/cocaine-core/cocaine-tskv.log |grep libmastermind | /usr/local/bin/libmastermind_cache.py  --ignore=cached_keys --ignore_namespaces=avatars-marketpictesting"
    - execution_interval: 300
    - execution_timeout: 60

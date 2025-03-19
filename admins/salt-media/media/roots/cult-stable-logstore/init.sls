include:
  - templates.push-server.server

timetail -n 3600 -t java /opt/storage/{{ salt['grains.get']('fqdn') }}/kassa_back-stable/*/yandex/tickets/api.log /opt/storage/{{ salt['grains.get']('fqdn') }}/kassa_generator/*/yandex/tickets/worker.log /opt/storage/{{ salt['grains.get']('fqdn') }}/kassa_generator/*/yandex/tickets/workercritical.log /opt/storage/{{ salt['grains.get']('fqdn') }}/kassa_admin/*/yandex/tickets/cms.log > /var/tmp/kassa-aggregate-logs.last1hr-new && mv /var/tmp/kassa-aggregate-logs.last1hr-new /var/tmp/kassa-aggregate-logs.last1hr:
  cron.present:
    - identifier: KASSASERVICES
    - user: root
    - minute: '*/30'

/etc/monrun/conf.d:
  file.recurse:
    - source: salt://cult-stable-logstore/files/etc/monrun/conf.d
    - user: root
    - group: root
    - file_mode: 644
    - makedirs: True

/etc/syslog-ng/conf.d:
  file.recurse:
    - source: salt://cult-stable-logstore/files/etc/syslog-ng/conf.d
    - user: root
    - group: root
    - file_mode: 644
    - makedirs: True

/opt/storage/syslog:
  file.directory:
    - user: root
    - group: root
    - mode: 755
    - makedirs: True

/etc/logrotate.d/syslog-ng-logs:
  file.managed:
    - source: salt://cult-stable-logstore/files/etc/logrotate.d/syslog-ng-logs
    - user: root
    - group: root
    - file_mode: 644
    - makedirs: True

/home:
  mount.mounted:
    - device: /var/home
    - fstype: none
    - opts: bind
    - mkmnt: True
    - persist: True

/etc/monitoring/watchdog.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 0644
    - makedirs: True
    - contents: /push-server-lite/ warning

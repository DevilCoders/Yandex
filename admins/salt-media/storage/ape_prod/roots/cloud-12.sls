{% set cgroup = grains['conductor']['group'] %}
{% set env = grains['yandex-environment'] %}
{% if env == 'production' or env == 'prestable' %}
{% set pgenv = 'production' %}
{% else %}
{% set pgenv = 'testing' %}
{% endif %}

/var/cache/cocaine/tvm:
    file.directory:
      - user: cocaine
      - dir_mode: 755

/var/cache/mastermind:
    file.directory:
      - user: cocaine
      - dir_mode: 755

cocaine_to_karl_group:
  group.present:
    - name: karl
    - addusers:
      - cocaine

/etc/monrun/conf.d/fd.conf:
  file.managed:
    - source: salt://common-files/etc/monrun/conf.d/fd.conf

/etc/cocaine/mecoll.yaml:
  file.managed:
    - source: salt://cloud-12/etc/cocaine/mecoll.yaml
    - makedirs: True

/etc/logrotate.conf:
  file.managed:
    - source: salt://common-files/etc/logrotate.conf

/etc/cocaine/.cocaine/tools.yml:
  file.managed:
    - contents_pillar: yavape:cocaine.tools.yml
    - user: cocaine
    - mode: 444
    - makedirs: True

/home/cocaine/.pgpass:
  file.managed:
    - contents_pillar: yavape:cocaine.pgpass.{{ pgenv }}
    - user: cocaine
    - mode: 600

/etc/sudoers.d/:
  file.recurse:
    - source: salt://cloud-12/etc/sudoers.d/
    - user: root
    - group: root
    - file_mode: 440

/var/lib/push-client-cocaine-backend:
    file.directory:
      - user: statbox
      - group: statbox
      - dir_mode: 755

/usr/local/bin/:
  file.recurse:
    - source: salt://cloud-12/usr/local/bin/
    - file_mode: '0755'

/usr/local/bin/qloud-uptime.py:
  file.managed:
    - source: salt://common-files/usr/local/bin/qloud-uptime.py
    - mode: '0755'

/usr/local/bin/cocaine-warmup.py:
  file.managed:
    - source: salt://cocaine-common/usr/local/bin/cocaine-warmup.py
    - mode: 755

/usr/local/bin/cocaine-graceful-restart.sh:
  file.managed:
    - source: salt://cocaine-common/usr/local/bin/cocaine-graceful-restart.sh
    - mode: 755

/usr/local/bin/actualize_interface.sh:
  file.managed:
    - source: salt://files/usefull_scripts/actualize_interface.sh
    - mode: 755

/etc/cron.d/:
  file.recurse:
    - source: salt://cloud-12/etc/cron.d/

/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py:
  file.managed:
    - source: salt://common-files/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py
    - mode: 755

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://cloud-12/cocaine.conf
    - template: jinja

/etc/monrun/conf.d:
  file.recurse:
    - source: salt://cloud-12/etc/monrun/conf.d/

/etc/logrotate.d/:
  file.recurse:
    - source: salt://cloud-12/etc/logrotate.d/

/usr/lib/yandex-3132-cgi/cocaine-tool-info:
  file.managed:
    - source: salt://cloud-12/usr/lib/yandex-3132-cgi/cocaine-tool-info
    - mode: 755

/etc/yandex/loggiver/loggiver.pattern:
  file.managed:
    - source: salt://common-files/etc/yandex/loggiver/loggiver.pattern

/etc/nginx/:
  file.recurse:
    - source: salt://cloud-12/etc/nginx/

/var/log/cocaine-runtime/:
  file.directory:
  - user: cocaine
  - group: adm
  - dir_mode: 755

cocaine-runtime:
  service:
    - disabled

include:
  - templates.elliptics-tls
  - templates.yasmagentnew
  - templates.distributed-flock
  - templates.cocaine-crashlog-clean
  - templates.push-client
  - templates.mavrodi-tls
  - templates.karl-tls
  - units.netconfiguration
  - units.iface-ip-conf
  - units.cocaine
  - units.walle_juggler_checks

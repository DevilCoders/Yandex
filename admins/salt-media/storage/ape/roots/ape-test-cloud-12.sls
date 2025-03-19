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

/etc/sudoers.d:
  file.recurse:
    - source: salt://ape-test-cloud-12/etc/sudoers.d/
    - user: root
    - group: root
    - file_mode: 440

/etc/cron.d:
  file.recurse:
    - source: salt://ape-test-cloud-12/etc/cron.d/

/etc/monrun/conf.d:
  file.recurse:
    - source: salt://ape-test-cloud-12/etc/monrun/conf.d/

/usr/local/bin:
  file.recurse:
    - source: salt://ape-test-cloud-12/usr/local/bin/
    - file_mode: '0755'

/usr/local/bin/cocaine-warmup.py:
  file.managed:
    - source: salt://test-cocaine-common/usr/local/bin/cocaine-warmup.py
    - mode: 755

/usr/local/bin/cocaine-graceful-restart.sh:
  file.managed:
    - source: salt://test-cocaine-common/usr/local/bin/cocaine-graceful-restart.sh
    - mode: 755

/etc/yandex/loggiver/loggiver.pattern:
  file.managed:
    - source: salt://common-files/etc/yandex/loggiver/loggiver.pattern

/etc/logrotate.d:
  file.recurse:
    - source: salt://ape-test-cloud-12/etc/logrotate.d/

/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py:
  file.managed:
    - source: salt://common-files/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py
    - mode: 755

/var/run/cocaine:
  file.directory:
  - user: cocaine
  - group: adm
  - dir_mode: 755
  - file_mode: 644

/var/log/cocaine-runtime:
  file.directory:
  - user: cocaine
  - group: adm
  - dir_mode: 755
  - file_mode: 644

/etc/apt/sources.list.d/cocaine.list:
  file.managed:
    - source: salt://cocaine.list

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://ape-test-cloud-12/etc/cocaine/cocaine.conf
    - template: jinja

/etc/cocaine/auth.keys:
  file.managed:
    - source: salt://ape-test-cloud-12/etc/cocaine/auth.keys

cocaine-runtime:
  service:
    - disabled

/usr/lib/yandex-3132-cgi/cocaine-tool-info:
  file.managed:
    - source: salt://ape-test-cloud-12/usr/lib/yandex-3132-cgi/cocaine-tool-info
    - mode: 755

include:
  - templates.elliptics-tls
  - templates.yasmagentnew
  - templates.push-client
  - templates.mavrodi-tls
  - templates.karl-tls
  - units.netconfiguration
  - units.iface-ip-conf
  - units.cocaine
  - units.juggler-porto-fix

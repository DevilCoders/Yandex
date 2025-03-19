{% set cgroup = grains['conductor']['group'] %}
{% set env = grains['yandex-environment'] %}
{% if env == 'production' or env == 'prestable' %}
{% set pgenv = 'production' %}
{% else %}
{% set pgenv = 'testing' %}
{% endif %}

include:
  - units.kernel_tune
  - units.udev_tune
  - templates.yasmagent
  - templates.push-client
  - templates.mavrodi-tls
  - units.netconfiguration
  - units.iface-ip-conf
  - units.yarl

/etc/monrun/conf.d/fd.conf:
  file.managed:
    - source: salt://common-files/etc/monrun/conf.d/fd.conf

/etc/distributed-flock-media.json:
  file.managed:
    - source: salt://ape-test-front-12/etc/distributed-flock-media.json

/etc/cocaine-http-proxy/cocaine-http-proxy.yml:
  file.managed:
    - contents_pillar: yavape:cocaine.cocaine-http-proxy.yml-testing
    - makedirs: True

/etc/ubic/service/cocaine-http-proxy:
  file.managed:
    - source: salt://ape-test-front-12/etc/ubic/service/cocaine-http-proxy

/etc/cocaine/.cocaine/tools.yml:
  file.managed:
    - contents_pillar: yavape:cocaine.tools.yml
    - user: cocaine
    - mode: 444
    - makedirs: True

/etc/cocaine/auth.keys:
  file.managed:
    - source: salt://ape-test-cloud-12/etc/cocaine/auth.keys

/home/cocaine/.pgpass:
  file.managed:
    - contents_pillar: yavape:cocaine.pgpass.{{ pgenv }}
    - user: cocaine
    - mode: 600

/etc/monrun/conf.d/:
  file.recurse:
    - source: salt://ape-test-front-12/etc/monrun/conf.d/

/usr/local/bin/:
  file.recurse:
    - source: salt://ape-test-front-12/usr/local/bin/
    - file_mode: '0755'

/usr/local/bin/cocaine-warmup.py:
  file.managed:
    - source: salt://test-cocaine-common/usr/local/bin/cocaine-warmup.py
    - mode: 755

/usr/local/bin/cocaine-graceful-restart.sh:
  file.managed:
    - source: salt://test-cocaine-common/usr/local/bin/cocaine-graceful-restart.sh
    - mode: 755

/etc/cron.d:
  file.recurse:
    - source: salt://ape-test-front-12/etc/cron.d

/etc/apt/sources.list.d/cocaine.list:
  file.managed:
    - source: salt://cocaine.list

/etc/sysctl.d/zz-cocaine.conf:
  file.managed:
    - source: salt://ape-test-front-12/etc/sysctl.d/zz-cocaine.conf

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://ape-test-front-12/etc/nginx/nginx.conf

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://ape-test-front-12/etc/nginx/sites-enabled

/etc/nginx/common:
  file.recurse:
    - source: salt://ape-test-front-12/etc/nginx/common

/etc/nginx/tvm:
  file.recurse:
    - source: salt://ape-test-front-12/etc/nginx/tvm

/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py:
  file.managed:
    - source: salt://common-files/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py
    - mode: 755

/etc/yandex/loggiver/loggiver.pattern:
  file.managed:
    - source: salt://common-files/etc/yandex/loggiver/loggiver.pattern

/var/cache/nginx/cache:
  file.directory:
  - user: www-data
  - group: www-data
  - dir_mode: 755
  - file_mode: 644

/etc/nginx/conf.d:
  file.recurse:
    - source: salt://ape-test-front-12/etc/nginx/conf.d

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://ape-test-front-12/etc/cocaine/cocaine.conf
    - template: jinja

/etc/logrotate.d/cocaine-runtime:
  file.managed:
    - source: salt://logrotate.d/cocaine-runtime

/etc/logrotate.d/cocaine-tornado-proxy:
  file.managed:
    - source: salt://logrotate.d/cocaine-tornado-proxy

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://logrotate.d/nginx

/etc/default:
  file.recurse:
    - source: salt://ape-test-front-12/etc/default

/etc/cocaine-tornado-proxy/cocaine-tornado-proxy-testing.conf:
  file.managed:
    - contents_pillar: yavape:cocaine.cocaine-tornado-proxy-testing.conf
    - mode: 644
    - makedirs: True

/etc/cocaine-tornado-proxy/srwconfig.conf:
  file.managed:
    - source: salt://ape-test-front-12/etc/cocaine-tornado-proxy/srwconfig.conf

/var/log/cocaine-runtime:
  file.directory:
    - user: cocaine
    - group: adm
    - mode: 755
    - makedirs: True

nginx:
  service:
    - running
    - enable: True
    - reload: True
    - watch:
      - file: /etc/nginx/sites-enabled

cocaine-runtime:
  service:
    - disabled

/etc/nginx/ssl/certuml4.pem:
  file.managed:
    - contents_pillar: yavape:cocaine.ape-test-front-12.certuml4.pem
    - makedirs: True
    - user: root
    - mode: 600

/etc/nginx/ssl/front.key:
  file.managed:
    - contents_pillar: yavape:cocaine.ape-test-front-12.front.key
    - makedirs: True
    - user: root
    - mode: 600

/etc/nginx/ssl/yapic-testkey.pem:
  file.managed:
    - contents_pillar: yavape:cocaine.ape-test-front-12.yapic-testkey.pem
    - makedirs: True
    - user: root
    - mode: 600

/etc/nginx/include/tikaite_common.conf:
  file.managed:
    - source: salt://ape-test-front-12/etc/nginx/include/tikaite_common.conf
    - makedirs: True


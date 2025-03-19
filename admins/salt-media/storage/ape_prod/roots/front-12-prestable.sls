{% set cgroup = grains['conductor']['group'] %}
{% set env = grains['yandex-environment'] %}
{% if env == 'production' or env == 'prestable' %}
{% set pgenv = 'production' %}
{% else %}
{% set pgenv = 'testing' %}
{% endif %}

include:
  - templates.distributed-flock
  - templates.push-client
  - templates.mavrodi-tls
  - units.netconfiguration
  - units.iface-ip-conf
  - units.kernel_tune
  - units.udev_tune
  - units.walle_juggler_checks
  - units.tls_session_tickets
  - units.yarl
  - templates.libmastermind_cache

libmastermind_cache:
  monrun.present:
    - command: "timetail -t java /var/log/s3/goose-maintain/libmds.log |grep libmastermind | /usr/local/bin/libmastermind_cache.py  --ignore=cached_keys"
    - execution_interval: 300
    - execution_timeout: 60
    - type: mediastorage-proxy


/etc/distributed-flock-media.json:
  file.managed:
    - source: salt://storage/etc/distributed-flock-media.json

/etc/monrun/conf.d/fd.conf:
  file.managed:
    - source: salt://common-files/etc/monrun/conf.d/fd.conf

/etc/cocaine/mecoll.yaml:
  file.managed:
    - source: salt://front-12-prestable/etc/cocaine/mecoll.yaml
    - makedirs: True

/home/robot-media-salt/.ssh/id_rsa:
  file.managed:
    - contents_pillar: yavape:cocaine.robot-media-salt.ssh.id_rsa
    - user: robot-media-salt
    - mode: 600
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

/etc/nginx/sites-enabled/:
  file.recurse:
    - source: salt://front-12-prestable/etc/nginx/sites-enabled/

/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py:
  file.managed:
    - source: salt://common-files/usr/lib/yandex-graphite-checks/enabled/cocaine-logs-metrics.py
    - mode: 755

/etc/ubic/service/cocaine-runtime.ini:
  file.managed:
    - source: salt://front-12/etc/ubic/service/cocaine-runtime.ini

/etc/cocaine-http-proxy/cocaine-http-proxy.yml:
  file.managed:
    - contents_pillar: yavape:cocaine.cocaine-http-proxy.yml-prestable
    - makedirs: True

/etc/ubic/service/cocaine-http-proxy:
  file.managed:
    - source: salt://front-12/etc/ubic/service/cocaine-http-proxy

/etc/nginx/include/tikaite_common.conf:
  file.managed:
    - source: salt://front-12/etc/nginx/include/tikaite_common.conf
    - makedirs: True

/etc/sysctl.d/:
  file.recurse:
    - source: salt://front-12/etc/sysctl.d/

/usr/local/bin/:
  file.recurse:
    - source: salt://front-12-prestable/usr/local/bin/
    - file_mode: '0755'

/usr/local/bin/cocaine-warmup.py:
  file.managed:
    - source: salt://cocaine-common/usr/local/bin/cocaine-warmup.py
    - mode: 755

/usr/local/bin/socketleak-autofix.sh:
  file.managed:
    - source: salt://front-12/usr/local/bin/socketleak-autofix.sh
    - mode: 755

/usr/local/bin/tornado-proxy-autofix.sh:
  file.managed:
    - source: salt://front-12/usr/local/bin/tornado-proxy-autofix.sh
    - mode: 755

/usr/local/bin/unicorn-autofix.sh:
  file.managed:
    - source: salt://front-12/usr/local/bin/unicorn-autofix.sh
    - mode: 755

/usr/local/bin/cocaine-graceful-restart.sh:
  file.managed:
    - source: salt://cocaine-common/usr/local/bin/cocaine-graceful-restart.sh
    - mode: 755

/etc/cron.d/:
  file.recurse:
    - source: salt://front-12-prestable/etc/cron.d/

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://front-12-prestable/cocaine.conf
    - template: jinja

/etc/logrotate.d/cocaine-runtime:
  file.managed:
    - source: salt://logrotate.d/cocaine-runtime

/etc/logrotate.d/cocaine-tornado-proxy:
  file.managed:
    - source: salt://logrotate.d/cocaine-tornado-proxy

/etc/logrotate.d/statbox:
  file.managed:
    - source: salt://logrotate.d/statbox

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://logrotate.d/nginx

/etc/nginx/common:
  file.recurse:
    - source: salt://front-12-prestable/etc/nginx/common/

/usr/sbin/get-secdist-keys.sh:
  file.managed:
    - source: salt://front-12/etc/nginx/tls_ticket/get-secdist-keys.sh

/etc/cron.d/tls_ticket_cron:
  file.managed:
    - source: salt://front-12/etc/nginx/tls_ticket/tls_ticket_cron

/etc/nginx/nginx.conf:
  file.managed:
    - source: salt://front-12-prestable/etc/nginx/nginx.conf

/etc/nginx/conf.d/01-accesslog.conf:
  file.managed:
    - source: salt://front-12-prestable/etc/nginx/conf.d/01-accesslog.conf

/etc/nginx/secret_start_internal.conf:
  file.managed:
    - contents_pillar: yavape:cocaine.secret_start_internal.conf

/etc/yandex/loggiver/loggiver.pattern:
  file.managed:
    - source: salt://common-files/etc/yandex/loggiver/loggiver.pattern

/etc/monrun/conf.d/:
  file.recurse:
    - source: salt://front-12/etc/monrun/conf.d/
    - template: jinja

/etc/cocaine-tornado-proxy/cocaine-tornado-proxy-prestable.conf:
  file.managed:
    - contents_pillar: yavape:cocaine.cocaine-tornado-proxy-prestable.conf

/etc/cocaine-tornado-proxy/srwconfig.conf:
  file.managed:
    - source: salt://front-12-prestable/etc/cocaine-tornado-proxy/srwconfig.conf

/etc/default/:
  file.recurse:
    - source: salt://front-12-prestable/etc/default/

/var/log/cocaine-runtime/:
  file.directory:
  - user: cocaine
  - group: adm
  - dir_mode: 755

/var/cache/cocaine/meta-mastermind:
  file.directory:
  - user: cocaine
  - group: adm
  - dir_mode: 755

cocaine-runtime:
  service:
    - disabled

/etc/nginx/ssl/certuml4.pem:
  file.managed:
    - contents_pillar: yavape:cocaine.ape-test-front-12.certuml4.pem
    - user: root
    - mode: 600

/etc/nginx/ssl/dhparam.pem:
  file.managed:
    - contents_pillar: yavape:cocaine.ape-front-12.dhparam.pem
    - user: root
    - mode: 600

/etc/nginx/tvm/asymmetric.public:
  file.managed:
    - contents_pillar: yavape:cocaine.asymmetric.public
    - user: root
    - mode: 600

/var/cache/nginx/cache:
  mount.mounted:
    - device: tmpfs
    - fstype: tmpfs
    - opts: defaults,nodev,nosuid,size=20971520k
    - persist: True

/etc/default/yasmagent:
  file.managed:
    - source: salt://front-12/etc/default-custom/yasmagent
    - mode: 644
    - user: root
    - group: root
    - template: jinja

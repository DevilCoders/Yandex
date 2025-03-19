{% set cgroup = grains['conductor']['group'] %}
{% set env = grains['yandex-environment'] %}
{% if env == 'production' or env == 'prestable' %}
{% set pgenv = 'production' %}
{% else %}
{% set pgenv = 'testing' %}
{% endif %}

include:
  - units.netconfiguration
  - units.iface-ip-conf

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

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://ape-load-cloud/etc/cocaine/cocaine.conf
    - template: jinja

/etc/cocaine/auth.keys:
  file.managed:
    - source: salt://ape-load-cloud/etc/cocaine/auth.keys

/etc/cocaine-isolate-daemon/cocaine-isolate-daemon.conf:
  file.managed:
    - source: salt://cloud-12/etc/cocaine-isolate-daemon/cocaine-isolate-daemon.conf
    - makedirs: True

cocaine-runtime:
  service:
    - disabled

/usr/lib/yandex-3132-cgi/cocaine-tool-info:
  file.managed:
    - source: salt://cloud-12/usr/lib/yandex-3132-cgi/cocaine-tool-info
    - mode: 755


/etc/logrotate.d:
  file.recurse:
    - source: salt://ape-load-cloud/etc/logrotate.d/

/var/log/cocaine-runtime:
  file.directory:
    - user: cocaine
    - group: adm
    - mode: 755
    - makedirs: True

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

/etc/cocaine/auth.keys:
  file.managed:
    - source: salt://ape-load-cloud/etc/cocaine/auth.keys

/etc:
    file.recurse:
      - source: salt://ape-load-front/etc
      - include_empty: True

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://ape-load-front/etc/cocaine/cocaine.conf
    - template: jinja

/etc/logrotate.d/nginx:
  file.managed:
    - source: salt://logrotate.d/nginx

/etc/logrotate.d/cocaine-runtime:
  file.managed:
    - source: salt://logrotate.d/cocaine-runtime

cocaine-runtime:
  service:
    - disabled

/var/log/cocaine-runtime:
  file.directory:
    - user: cocaine
    - group: adm
    - mode: 755
    - makedirs: True

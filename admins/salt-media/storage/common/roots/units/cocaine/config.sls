{% set cgroup = grains['conductor']['group'] %}
{% set env = grains['yandex-environment'] %}
{% if env == 'production' or env == 'prestable' %}
{% set pgenv = 'production' %}
{% else %}
{% set pgenv = 'testing' %}
{% endif %}

{%- from slspath + "/map.jinja" import config with context -%}

#ACCEPT TO ALL
/etc/cocaine/auth.keys:
  file.managed:
    - source: salt://units/cocaine/files/etc/cocaine/auth.keys
    - user: cocaine
    - group: adm
    - makedirs: True
    - mode: 664

/var/cache/cocaine/tvm:
  file.directory:
  - user: cocaine
  - group: adm
  - dir_mode: 760
  - file_mode: 640
  - makedirs: True

/etc/cocaine/mecoll.yaml:
  file.managed:
    - source: salt://units/cocaine/files/etc/cocaine/mecoll.yaml
    - template: jinja
    - user: root
    - group: root
    - mode: 644

/etc/logrotate.d/cocaine-core:
  file.managed:
    - source: salt://units/cocaine/files/etc/logrotate.d/cocaine-core
    - user: root
    - group: root
    - mode: 644


#ELLIPTICS:STORAGE
{% if 'elliptics-storage' in cgroup or 'elliptics-test-storage' in cgroup or 'elliptics-storage-federations' in cgroup or 'elliptics-test-storage-federations' in cgroup %}

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://units/cocaine/files/etc/cocaine/cocaine.conf.srw
    - template: jinja
    - context:
        config: {{ config }}

/etc/cocaine/.cocaine/tools.yml:
  file.managed:
    - contents_pillar: yav:cocaine.tools.yml
    - user: cocaine
    - mode: 444
    - makedirs: True

/home/cocaine/.pgpass:
  file.managed:
    - contents_pillar: yav:cocaine.pgpass.{{ pgenv }}
    - user: cocaine
    - mode: 600

{% if config.mavrodi.enabled -%}
include:
  - templates.mavrodi-tls
{%- endif %}

#ELLIPTICS:PROXY
{% elif cgroup == 'elliptics-test-proxies' or 'elliptics-proxy' in cgroup %}

/etc/cocaine/cocaine.conf:
  file.managed:
    - source: salt://units/cocaine/files/etc/cocaine/cocaine.conf.proxy
    - template: jinja
    - context:
        config: {{ config }}

/etc/cocaine/.cocaine/tools.yml:
  file.managed:
    - contents_pillar: yav:cocaine.tools.yml
    - user: cocaine
    - mode: 444
    - makedirs: True

#ELLIPTICS:MASTERMIND
{% elif cgroup == 'elliptics-cloud' or cgroup == 'elliptics-test-cloud' or cgroup == 'elliptics-test-collector' or cgroup == 'elliptics-collector' or cgroup == 'elliptics-cloud-prestable' or cgroup == 'elliptics-collector-prestable' %}

# now deployed from pkg "photo-cocaine-conf"
#/etc/cocaine/cocaine.conf:
#  file.managed:
#    - source: salt://cocaine/files/etc/cocaine/cocaine.conf.mastermind
#    - user: cocaine
#    - group: root
#    - mode: 644

/usr/local/bin/cocaine-warmup.py:
  file.managed:
    - source: salt://units/cocaine/files/usr/local/bin/cocaine-warmup.py.mastermind
    - user: root
    - group: root
    - mode: 755

/etc/cron.d/cocaine-warmup:
 file.managed:
    - source: salt://units/cocaine/files/etc/cron.d/cocaine-warmup
    - user: root
    - group: root
    - mode: 644

{% endif %}


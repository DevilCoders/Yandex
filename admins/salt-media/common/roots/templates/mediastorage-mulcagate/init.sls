{%- from slspath + "/map.jinja" import mediastorage with context -%}

/etc/elliptics/mediastorage-proxy.conf:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/elliptics/mediastorage-proxy.conf
    - mode: 644
    - user: root
    - group: root
    - template: jinja
    - makedirs: true
    - context:
      mediastorage: {{ mediastorage }}

{% for dir in ['/var/log/mds',
               '/var/cache/mds_tvm_cache/',
               '/var/cache/libmastermind/',
               '/var/cache/libmastermind/meta-mastermind',
               '/var/cache/nscfg/',
               '/var/cache/proxy-acl/'] %}
{{ dir }}:
  file.directory
{% endfor %}

/var/cache/mastermind:
  file.directory:
    - user: root
    - mode: 1777
    - group: root
    - makedirs: True

/var/cache/proxy-acl/cache:
  file.managed:
    - mode: 644
    - user: root
    - group: root

/etc/logrotate.d/mediastorage-mulcagate:
  yafile.managed:
    - source: salt://{{ slspath }}/files/etc/logrotate.d/mediastorage-mulcagate
    - mode: 644
    - user: root
    - group: root
    - makedirs: true

mediastorage-proxy-pkg:
  pkg.installed:
    - name: 'mediastorage-proxy'

# Parsers
/usr/local/bin/thevoid.stat-access.py:
  yafile.managed:
    - source: salt://{{ slspath }}/files/thevoid.stat-access.py
    - mode: 755
    - user: root
    - group: root
    - makedirs: true

/etc/yandex/loggiver/pattern.d/elliptics-read-file-status.pattern:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/yandex/loggiver/pattern.d/elliptics-read-file-status.pattern
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

libmastermind_cache:
  monrun.present:
    - command: "timetail -t java {{ mediastorage.log.path }} |grep libmastermind | /usr/local/bin/libmastermind_cache.py  --ignore=cached_keys"
    - execution_interval: 300
    - execution_timeout: 60
    - type: mediastorage-proxy

/usr/local/bin/mds-proxy-restarter.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/mds-proxy-restarter.sh
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
    - template: jinja
    - context:
      mediastorage: {{ mediastorage }}

mds-proxy-restarter-logrotate:
  file.managed:
    - name: /etc/logrotate.d/mds-proxy-restarter
    - contents: |
        /var/log/mds/restart.log
        {
            size 100M
            missingok
            rotate 7
            compress
            compresscmd /usr/bin/pzstd
            compressoptions -8 -p1
            compressext .zst
            dateext
            dateformat .%Y%m%d-%s
            notifempty
        }

{% if mediastorage.use_file %}
nginx-maintenance-file:
  monrun.present:
    - command: "if [[ -e /etc/nginx/maintenance.file ]]; then file=$(($(date +%s)-$(stat -c %X /etc/nginx/maintenance.file))); if [[ $file -gt 86400 ]]; then echo '2;maintenance file is older than 1 day'; elif [[ $file -gt 3600 ]]; then echo '1;maintenance file is older than 1 hour'; else echo '0;OK'; fi; else echo '0;OK'; fi"
    - execution_interval: 600
    - type: mediastorage-proxy
{% endif %}

mediastorage-proxy:
  monrun.present:
    - command: "/usr/bin/jhttp.sh -u /gate/get/{{ mediastorage.ping_stid }} -p {{ mediastorage.port }}"
    - execution_interval: 60
    - type: mediastorage-proxy

yandex-ubic-mulcagate:
  pkg.purged:
    - version: "0.4"

include:
  - templates.elliptics-tls
  - templates.libmastermind_cache
  {% if mediastorage.mavrodi.enabled -%}
  - templates.mavrodi-tls
  {%- endif %}
  {% if mediastorage.karl.tls.enabled -%}
  - templates.karl-tls
  {%- endif %}

# MDS-13384
nodes_alive:
  file.managed:
    - name: /usr/local/bin/nodes_alive.sh
    - source: salt://{{ slspath }}/files/nodes_alive.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: true

  monrun.present:
    - command: "/usr/local/bin/nodes_alive.sh {{ mediastorage.nodes }}"
    - execution_interval: 300
    - execution_timeout: 60
    - type: mediastorage-proxy

{%- set log_dir = salt['pillar.get']('s3-dbutils:dirs:log_dir', '') -%}
{%- set lock_dir = salt['pillar.get']('s3-dbutils:dirs:lock_dir', '') -%}
{%- set status_dir = salt['pillar.get']('s3-dbutils:dirs:status_dir', '') -%}

{% set dirs = [log_dir, lock_dir, status_dir] %}

{% set files = [
    '/etc/logrotate.d/s3-dbutils',
    '/etc/cron.d/update_chunks_counters',
    '/etc/monrun/conf.d/s3-dbutils.conf'
  ]
%}

{% set securefiles = [
    '/etc/s3-dbutils/.pgpass'
  ]
%}

{% set exefiles = [
    '/usr/local/bin/s3dbutils_wrapper.sh',
    '/usr/local/bin/s3dbutils_mon.sh',
    '/usr/local/bin/pg_counters_queue.py',
    '/usr/local/bin/pg_counters_update.py',
  ]
%}

{% for file in files %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 644
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in securefiles %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 600
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for file in exefiles %}
{{file}}:
  yafile.managed:
    - source: salt://{{ slspath }}/files{{ file }}
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
    - template: jinja
{% endfor %}

{% for dir in dirs %}
{{dir}}:
  file.directory:
    - mode: 755
    - user: root
    - group: root
    - makedirs: True
{% endfor %}

dependencies:
  pkg.installed:
    - pkgs:
      - zk-flock
      - python-psycopg2
      - s3-dbutils


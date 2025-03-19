{% set cron = salt['pillar.get']('dynamic_roots:cron') %}
{% set log_file = salt['pillar.get']('dynamic_roots:log_file', '/dev/null') %}
{% set user = salt['pillar.get']('dynamic_roots:user', 'root') %}
{% set dirs = [
  salt['pillar.get']('dynamic_roots:config:git_mirrors_dir'),
  salt['pillar.get']('dynamic_roots:config:git_checkouts_dir'),
  salt['file.dirname'](salt['pillar.get']('dynamic_roots:config:file_roots_config_path')),
  salt['file.dirname'](salt['pillar.get']('dynamic_roots:config:pillar_roots_config_path')),
  '/var/tmp/salt-dynamic-roots',
] %}

/srv/salt/extmods/pillar/dynamic_roots.py:
  file.managed:
    - source: salt://{{slspath}}/files/extmods/pillar/dynamic_roots.py
    - mode: 644
    - makedirs: True
    - dir_mode: 755

/srv/salt/extmods/fileserver/dynamic_roots.py:
  file.managed:
    - source: salt://{{slspath}}/files/extmods/fileserver/dynamic_roots.py
    - mode: 644
    - makedirs: True
    - dir_mode: 755

{% if cron %}
{% for d in (dirs | unique) %}
'{{ d }}':
  file.directory:
    - mode: 0755
    - user: {{ user }}
{% endfor %}

{% if log_file %}
{{ log_file }}:
  file.managed:
    - mode: 644
    - user: {{ user }}
/etc/logrotate.d/salt-dynamic-roots:
  file.managed:
    - makedirs: True
    - user: root
    - mode: 644
    - contents: |
        {{ log_file }}
        {
            weekly
            maxsize 300M
            compress
            dateext
            dateformat -%Y%m%d-%s
            missingok
            copytruncate
            rotate 15
        }
{% endif %}

/etc/yandex/salt-dynamic-roots/config.yaml:
  file.serialize:
    - makedirs: True
    - user: root
    - mode: 644
    - formatter: yaml
    - dataset_pillar: dynamic_roots:config
    - serializer_opts:
      - default_flow_style: False
      - indent: 4

/usr/local/bin/salt-dynamic-roots.py:
  file.managed:
    - source: salt://{{slspath}}/files/bin/salt-dynamic-roots.py
    - user: root
    - group: root
    - mode: 755
    - makedirs: True
    - dir_mode: 755

/etc/cron.d/salt-dynamic-roots:
  file.managed:
    - makedirs: True
    - user: root
    - mode: 644
    - contents: |
        {{ cron }} flock -x /var/tmp/salt-dynamic-roots/cron-lock /usr/local/bin/salt-dynamic-roots.py /etc/yandex/salt-dynamic-roots/config.yaml >> {{ log_file }} 2>&1
{% endif %}

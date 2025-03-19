{% from slspath + "/map.jinja" import salt_rsync with context %}
{% if salt_rsync.master_group is defined %}
{% if salt_rsync.master_group %}
{% set master_group = salt_rsync.master_group %}
{% else %}
{% set master_group = False %}
{% set hosts_allow = False %}
{% endif %}
{% else %}
{% set master_group = salt_rsync.project + '-' + salt_rsync.senv + '-salt' %}
{% endif %}
{% if hosts_allow is not defined %}
{% set hosts_allow = salt['cmd.run']('curl -s https://c.yandex-team.ru/api-cached/groups2hosts/' ~ master_group).replace('\n', ' ') %}
{% endif %}

include:
  - .services

{% for share_name, share_opts in salt_rsync.shares.items() %}
{% for opt in share_opts %}
{% for key, value in opt.items() %}
{% if key == 'path' %}
{{ value }}-share:
  file.directory:
    - name: {{ value }}
    - makedirs: True
{% endif %}
{% endfor %}
{% endfor %}
{% endfor %}

/etc/rsyncd.conf:
  file.managed:
    - source: {{ salt_rsync.config }}
    - template: jinja
    - context:
      user: {{ salt_rsync.user }}
      group: {{ salt_rsync.group }}
      log: {{ salt_rsync.log }}
      loglevel: {{ salt_rsync.loglevel }}
      shares: {{ salt_rsync.shares }}
      hosts_allow: {{ hosts_allow }}
      senv: {{ salt_rsync.senv }}
      project: {{ salt_rsync.project }}
    - makedirs: True
    - user: {{ salt_rsync.user }}
    - group: {{ salt_rsync.group }}
    - mode: 644
    - watch_in:
      - service: {{ salt_rsync.service }}

/etc/default/rsync:
  file.managed:
    - source: {{ salt_rsync.default_config }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - watch_in:
      - service: {{ salt_rsync.service }}

{{ salt_rsync.log }}:
  file.managed:
    - makedirs: True
    - user: {{ salt_rsync.user }}
    - group: {{ salt_rsync.group }}
    - mode: 644
    - watch_in:
      - service: {{ salt_rsync.service }}

{{ salt_rsync.logrotate_name }}:
  file.managed:
    - source: {{ salt_rsync.logrotate }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
      log: {{ salt_rsync.log }}

{% if hosts_allow %}
{% if salt['grains.get']('conductor:group').encode('utf8') == master_group %}
{{ salt_rsync.sync_script_name }}:
  file.managed:
    - source: {{ salt_rsync.sync_script }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 744
    - template: jinja
    - context:
      hosts_allow: {{ hosts_allow }}
      sync_log: {{ salt_rsync.sync_log }}

{{ salt_rsync.sync_log }}:
  file.managed:
    - makedirs: True
    - user: root
    - group: root
    - mode: 644

{{ salt_rsync.sync_logrotate_name }}:
  file.managed:
    - source: {{ salt_rsync.sync_logrotate }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
      log: {{ salt_rsync.sync_log }}

{{ salt_rsync.sync_cron_name }}:
  file.managed:
    - source: {{ salt_rsync.sync_cron }}
    - makedirs: True
    - user: root
    - group: root
    - mode: 644
    - template: jinja
    - context:
      sync_script: {{ salt_rsync.sync_script_name }}
{% else %}
{{ salt_rsync.sync_cron_name }}:
  file.absent
{% endif %}
{% endif %}

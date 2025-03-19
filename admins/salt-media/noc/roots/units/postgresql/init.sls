{% set postgresql = salt['pillar.get']('postgresql') %}
{% set config = salt['pillar.get']('postgresql:config', None) %}
{% set hba = salt['pillar.get']('postgresql:hba', None) %}
{% set users = salt['pillar.get']('postgresql:users', None) %}
{% set repmgr = salt['pillar.get']('postgresql:repmgr', None) %}
{% set conf_dir = '/etc/postgresql/' + postgresql ['version']|string + '/' + postgresql['cluster'] + '/' %}
{% set slave = salt['cmd.shell']('sudo -u postgres psql -tAc "SELECT pg_is_in_recovery()::int;"')|int %}

{% if salt['pillar.get']('postgresql:mdb_repo', True) %}
  {% set repo_name = 'mdb-' + grains['lsb_distrib_codename'] %}
  {% for arch in ['all', 'amd64'] %}
mdb-repo-{{ arch }}:
  pkgrepo.managed:
    - name: deb http://{{ repo_name }}.dist.yandex.ru/{{ repo_name }} stable/{{ arch }}/
    - file: /etc/apt/sources.list.d/{{ repo_name }}.list
  {% endfor %}
{% endif %}

postgresql-pkgs:
  pkg.installed:
    - pkgs:
      - postgresql-{{ postgresql['version'] }}
{% for ext in postgresql.get('extensions', []) %}
      - postgresql-{{ ext }}-{{ postgresql['version'] }}
{% endfor %}

{% if config %}
{{ conf_dir }}/conf.d/00-from_salt.conf:
  file.managed:
    - template: jinja
    - source: salt://units/postgresql/files/postgresql.conf
    - context:
      config: {{ config }}
{% endif %}

{% if hba %}
{{ conf_dir }}/pg_hba.conf:
  file.managed:
    - template: jinja
    - source: salt://units/postgresql/files/pg_hba_base.conf
    - context:
      hba: {{ hba }}
{% endif %}

{% if users and not slave %}
  {% for user in users %}
postgresql-user-{{ user['user'] }}:
  cmd.run:
    {% set options = user.get('options', [])|join(' ') %}
    {% if salt['cmd.shell']('sudo -u postgres psql -Atc "SELECT COUNT(rolname) FROM pg_roles WHERE rolname=\'' + user['user'] + '\'"')|int > 0 %}
    - name: sudo -u postgres psql -Atc "ALTER ROLE {{ user['user'] }} WITH PASSWORD '{{ user['password'] }}' {{  options }}"
    {% else %}
    - name: sudo -u postgres psql -Atc "CREATE ROLE {{ user['user'] }} WITH PASSWORD '{{ user['password'] }}' {{  options }}"
    {% endif %}
  {% endfor %}
{% endif %}

{% if repmgr %}
/var/lib/postgresql/.ssh/id_rsa:
  file.managed:
    - user: postgres
    - group: root
    - mode: 600
    - makedirs: True
    - contents: |
        {{ repmgr['ssh_key'] |indent(8) }}

/var/lib/postgresql/.pgpass:
  file.managed:
    - user: postgres
    - group: root
    - mode: 600
    - contents: |
        *:5432:repmgr:{{ repmgr['user'] }}:{{ repmgr['password'] }}
        *:5432:replication:{{ repmgr['user'] }}:{{ repmgr['password'] }}

/var/lib/postgresql/.ssh/id_rsa.pub:
  file.managed:
    - user: postgres
    - group: root
    - mode: 600
    - makedirs: True
    - contents: |
      {{ salt['cmd.shell']('ssh-keygen -y -f ~/.ssh/id_rsa') |indent(8) }}

repmgr-pkg:
  pkg.installed:
    - pkgs:
      - postgresql-{{ postgresql['version'] }}-repmgr

/etc/repmgr.conf:
  file.managed:
    - template: jinja
    - source: salt://units/postgresql/files/repmgr.conf
    - context:
      postgresql: {{ postgresql }}
      repmgr: {{ repmgr }}

/var/log/repmgr/repmgr.log:
  file.managed:
    - user: postgres
    - group: root
    - makedirs: True
    - mode: 644
{% endif %}

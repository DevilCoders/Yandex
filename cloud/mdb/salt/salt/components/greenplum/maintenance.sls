{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
gp_maintenance_dir:
  file.directory:
    - name: /usr/local/yandex/gp_maintenance
    - makedirs: True
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0755

get_catalog_table_list.sql:
  file.managed:
    - name: /usr/local/yandex/sqls/get_catalog_table_list.sql
    - mode: 0640
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - source: salt://{{ slspath }}/conf/scripts-sql/get_catalog_table_list.sql
    - template: jinja
    - require:
      - file: scripts-sqls-dir

get_table_list.sql:
  file.managed:
    - name: /usr/local/yandex/sqls/get_table_list_by_last_vacuum.sql
    - mode: 0640
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - source: salt://{{ slspath }}/conf/scripts-sql/get_table_list_by_last_vacuum.sql
    - template: jinja
    - require:
      - file: scripts-sqls-dir

maintenance_script:
  file.managed:
    - name: /usr/local/yandex/gp_maintenance/daily_operations.py
    - source: salt://{{ slspath }}/conf/scripts-maint/daily_operations.py
    - template: jinja
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0755
    - require:
      - file: gp_maintenance_dir

orphans_killer_script:
  file.managed:
    - name: /usr/local/yandex/orphans_killer.py
    - source: salt://{{ slspath }}/conf/orphans_killer.py
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0755

gp_is_master_script:
  file.managed:
    - name: /usr/local/yandex/gp_is_master
    - makedirs: True
    - user: root
    - group: root
    - mode: 0755
    - contents: |
        #!/bin/bash
        /usr/bin/timeout 1 {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/psql "user={{ gpdbvars.gpadmin }} dbname=postgres" -t -A -X -c 'SELECT 1' >/dev/null 2>&1

/etc/cron.d/gp_maintenance:
  file.managed:
    - source: salt://{{ slspath }}/conf/cron.d/gp_maintenance
    - template: jinja
    - mode: 0644
    - require:
      - file: get_catalog_table_list.sql
      - file: get_table_list.sql
      - file: maintenance_script
      - file: gp_is_master_script
{%   if salt.pillar.get('gpdb_master', False) or salt.grains.get('greenplum:role') == 'master' %}
      - create_gp_maintenance
{%   endif %}

/etc/cron.d/orphans_killer:
  file.managed:
    - source: salt://{{ slspath }}/conf/cron.d/orphans_killer
    - template: jinja
    - mode: 0644
    - require:
      - file: orphans_killer_script
      - file: gp_is_master_script

/etc/cron.d/disk_usage_watcher:
  file.managed:
    - source: salt://{{ slspath }}/conf/cron.d/disk_usage_watcher
    - mode: 644
    - template: jinja
    - require:
      - sls: components.greenplum.init_greenplum
      - file: /usr/local/yandex/disk_usage_watcher.py
      - file: gp_is_master_script

{% endif %}

/etc/logrotate.d/gp_maintenance.conf:
  file.absent

/usr/local/yandex/disk_usage_watcher.py:
  file.managed:
    - source: salt://{{ slspath }}/conf/disk_usage_watcher.py
    - mode: 755

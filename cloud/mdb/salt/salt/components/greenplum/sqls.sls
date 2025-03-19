{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}

scripts-sqls-dir:
    file.directory:
        - name: /usr/local/yandex/sqls
        - makedirs: True
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 700
        - require:
          - sls: components.greenplum.init_greenplum

mdb_toolkit_schema:
  file.managed:
    - name: /usr/local/yandex/sqls/mdb_toolkit.sql
    - mode: 0640
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - source: salt://{{ slspath }}/conf/scripts-sql/mdb_toolkit.sql
    - template: jinja
    - require:
      - file: scripts-sqls-dir

gp_terminate_cancel_backend:
  file.managed:
    - name: /usr/local/yandex/sqls/gp_terminate_cancel_backend.sql
    - mode: 0640
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - source: salt://{{ slspath }}/conf/scripts-sql/gp_terminate_cancel_backend.sql
    - template: jinja
    - require:
      - file: scripts-sqls-dir

gp_maintenance:
  file.managed:
    - name: /usr/local/yandex/sqls/gp_maintenance.sql
    - mode: 0640
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - source: salt://{{ slspath }}/conf/scripts-sql/maintenance.sql
    - template: jinja
    - require:
      - file: scripts-sqls-dir

gp_toolkit_perms:
  file.managed:
    - name: /usr/local/yandex/sqls/gp_toolkit_perms.sql
    - makedirs: True
    - mode: 0640
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - source: salt://{{ slspath }}/conf/scripts-sql/gp_toolkit_perms.sql
    - template: jinja

{%   if salt.pillar.get('gpdb_master', False) or salt.grains.get('greenplum:role') == 'master' %}
create_mdb_toolkit_schema:
  postgresql_cmd.psql_file:
    - runas: {{ gpdbvars.gpadmin }}
    - name: /usr/local/yandex/sqls/mdb_toolkit.sql
    - onchanges:
      - file: /usr/local/yandex/sqls/mdb_toolkit.sql
      - sls: components.greenplum.init_greenplum
    - require:
      - sls: components.greenplum.extensions
      - file: scripts-sqls-dir

create_gp_terminate_cancel_backend_functions:
  postgresql_cmd.psql_file:
    - runas: {{ gpdbvars.gpadmin }}
    - name: /usr/local/yandex/sqls/gp_terminate_cancel_backend.sql
    - onchanges:
      - file: /usr/local/yandex/sqls/gp_terminate_cancel_backend.sql
      - sls: components.greenplum.init_greenplum
    - require:
      - create_mdb_toolkit_schema

create_gp_maintenance:
  postgresql_cmd.psql_file:
    - runas: {{ gpdbvars.gpadmin }}
    - name: /usr/local/yandex/sqls/gp_maintenance.sql
    - onchanges:
      - file: /usr/local/yandex/sqls/gp_maintenance.sql
      - sls: components.greenplum.init_greenplum
    - require:
      - create_mdb_toolkit_schema

gp_toolkit_grant_perms:
  mdb_greenplum.gp_toolkit_grant_permissions:
    - runas: {{ gpdbvars.gpadmin }}
    - name: /usr/local/yandex/sqls/gp_toolkit_perms.sql
    - onchanges:
      - file: /usr/local/yandex/sqls/gp_toolkit_perms.sql
    - require:
      - file: scripts-sqls-dir
      - file: gp_toolkit_perms
{%   endif %}
{% endif %}

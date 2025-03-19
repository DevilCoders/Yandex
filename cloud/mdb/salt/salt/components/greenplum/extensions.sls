{% from "components/greenplum/map.jinja" import gpdbvars with context %}
{% set extension_databases = ['template1', 'postgres'] %}
{% set admin_role = 'mdb_admin' %}
{% if salt.pillar.get('gpdb_master') or salt.grains.get('greenplum:role') == 'master' %}

{% set mandatory_extensions = ['gp_pitr', 'gp_internal_tools'] %}
{% set protocols_need_granted = ['s3'] %}

{% if salt.pillar.get('data:pxf:install', True) %}
{%   do mandatory_extensions.append('pxf') %}
{%   do protocols_need_granted.append('pxf') %}
{% endif %}

{% set extensions = [] %}
{% for i in mandatory_extensions %}
{%   if i not in extensions %}
{% do extensions.insert(0, i) %}
{%   endif %}
{% endfor %}

mdb_admin_role:
    postgresql_cmd.psql_exec:
      - name: CREATE ROLE {{ admin_role }} WITH NOLOGIN;
      - runas: {{ gpdbvars.gpadmin }}
      - unless:
          - -tA -c "select 1 from pg_catalog.pg_roles where rolname = '{{ admin_role }}';" | grep -q "1"

{% for database in extension_databases %}
{%   for extension in extensions %}
{%     if extension %}
gp_extension_{{ extension }}_{{ database }}:
    mdb_greenplum.extension_present:
        - name: {{ extension }}
        - database: {{ database }}
        - require:
            - file: greenplum-service
        - require_in:
            - test: greenplum-ready
{%     endif %}

{%     if extension in protocols_need_granted %}
grant_{{ extension }}_to_mdb_admin-{{ database }}:
    postgresql_cmd.psql_exec:
        - name: grant all on protocol {{ extension }} to {{ admin_role }} with grant option
        - runas: {{ gpdbvars.gpadmin }}
        - maintenance_db: {{ database }}
        - unless:
          - -tA -c "select 1 from pg_extprotocol where ptcname = '{{ extension }}' and ptcacl::text~'{{ admin_role }}';" | grep -q "1"
        - require:
          - mdb_admin_role
          - gp_extension_{{ extension }}_{{ database }}
{%      endif %}
{%   endfor %}
{% endfor %}

## Protocols
{% for database in extension_databases %}
create_read_from_s3_func_{{ database }}:
      postgresql_cmd.psql_exec:
        - name: CREATE FUNCTION read_from_s3() RETURNS integer AS '$libdir/gps3ext.so', 's3_import' LANGUAGE C STABLE;
        - runas: {{ gpdbvars.gpadmin }}
        - maintenance_db: {{ database }}
        - unless:
            - -tA -c "select 'read_from_s3'::regproc;"

create_write_to_s3_func_{{ database }}:
      postgresql_cmd.psql_exec:
        - name: CREATE FUNCTION write_to_s3() RETURNS integer AS '$libdir/gps3ext.so', 's3_export' LANGUAGE C STABLE;
        - runas: {{ gpdbvars.gpadmin }}
        - maintenance_db: {{ database }}
        - unless:
            - -tA -c "select 'write_to_s3'::regproc;"

gp_protocol_s3_{{ database }}:
    mdb_greenplum.protocol_present:
        - name: s3
        - write_func: write_to_s3
        - read_func: read_from_s3
        - trusted: True
        - database: {{ database }}
        - require:
          - file: greenplum-service
          - create_read_from_s3_func_{{ database }}
          - create_write_to_s3_func_{{ database }}

grant_s3_to_mdb_admin_{{ database }}:
    postgresql_cmd.psql_exec:
        - name: grant all on protocol s3 to {{ admin_role }} with grant option
        - runas: {{ gpdbvars.gpadmin }}
        - maintenance_db: {{ database }}
        - unless:
          - -tA -c "select 1 from pg_extprotocol where ptcname = 's3' and ptcacl::text~'{{ admin_role }}';" | grep -q "1"
        - require:
          - mdb_admin_role
          - gp_protocol_s3_{{ database }}
{% endfor %}


{% if gpdbvars.version|int > 6192 or (gpdbvars.version|int == 6175 and gpdbvars.patch_level|int >= 103) or (gpdbvars.version|int == 6191 and gpdbvars.patch_level|int >= 109) %}
create_diskquota_db:
  mdb_greenplum.sync_database:
    - database: 'diskquota'
    - owner: 'gpadmin'
    - options:
        gp_default_storage_options: 'appendonly=false,blocksize=32768,compresstype=none,checksum=true,orientation=row'
{%   endif %}
        
{% endif %}


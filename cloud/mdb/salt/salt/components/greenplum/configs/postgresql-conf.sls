{% from "components/greenplum/map.jinja" import gpdbvars with context %}

include:
  - .postgresql-conf-no-deps
  
/usr/local/yandex/gp_wait_started.py:
  file.managed:
    - source: salt://components/greenplum/conf/gp_wait_started.py
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0755

{% if salt.pillar.get('gpdb_master', False) %}
{%   set configs_requirement = 'sls: components.greenplum.init_greenplum' %}
{% else %}
{%   set configs_requirement = 'cmd: gp-wait-started' %}
gp-wait-started:
  cmd.run:
{%   if salt['pillar.get']('restore-from:cid') %}
{# Disable the cluster wait timeout if we are restoring from the backup #}
    - name: /usr/local/yandex/gp_wait_started.py --wait 0
{%   else %}
    - name: /usr/local/yandex/gp_wait_started.py
{%   endif %}
    - runas: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - require:
      - sls: components.greenplum.walg
      - file: /usr/local/yandex/gp_wait_started.py
    - unless:
      - /usr/local/yandex/gp_wait_started.py -w 3
{% endif %}

extend:
{% for segment_info in salt['mdb_greenplum.get_segment_info']() %}
  {{ segment_info['datadir'] }}/postgresql.conf:
    file.managed:
      - require:
        - {{ configs_requirement }}
  {{ segment_info['datadir'] }}/pg_hba.conf:
    file.managed:
      - require:
        - {{ configs_requirement }}
{% endfor %}

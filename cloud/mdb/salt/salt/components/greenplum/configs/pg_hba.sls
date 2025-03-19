{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
{%   set pg_hba_template = "pg_hba.conf" %}
{% else %}
{%   set pg_hba_template = "pg_hba_segs.conf" %}
{% endif %}

{% for segment_info in salt['mdb_greenplum.get_segment_info']() %}
{{ segment_info['datadir'] }}/pg_hba.conf:
  file.managed:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0600
    - source: salt://components/greenplum/conf/gpconfigs/{{ pg_hba_template }}
    - template: jinja
    - create: False
{%    if salt['pillar.get']('service-restart', False) and salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}
    - require_in:
      - greenplum-stop
{%    endif %}

reload-segment-{{ segment_info['content']}}:
  mdb_greenplum.reload_segment:
    - content_id: {{ segment_info['content'] }}
    - port: {{ segment_info['port'] }}
    - onchanges:
      - file: {{ segment_info['datadir'] }}/pg_hba.conf
{% endfor %}

{% from "components/greenplum/map.jinja" import gpdbvars with context %}

include:
  - .pg_hba

{% for segment_info in salt['mdb_greenplum.get_segment_info']() %}
{{ segment_info['datadir'] }}/postgresql.conf:
  file.managed:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0600
    - source: salt://components/greenplum/conf/gpconfigs/postgresql.conf
    - template: jinja
    - makedirs: True
    - context:
      content_id: {{ segment_info['content'] }}
      port: {{ segment_info['port'] }}
      datadir: {{ segment_info['datadir'] }}
{%    if salt['pillar.get']('service-restart', False) and salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}
    - require_in:
      - greenplum-stop
{%    endif %}
    - create: False
{% endfor %}

extend:
{% for segment_info in salt['mdb_greenplum.get_segment_info']() %}
  reload-segment-{{ segment_info['content']}}:
    mdb_greenplum.reload_segment:
      - onchanges:
        - file: {{ segment_info['datadir'] }}/postgresql.conf
{% endfor %}

/etc/logrotate.d/greenplum-common:
  file.managed:
    - source: salt://components/greenplum/conf/greenplum.logrotate
    - template: jinja
    - user: root
    - group: root
    - mode: 0644

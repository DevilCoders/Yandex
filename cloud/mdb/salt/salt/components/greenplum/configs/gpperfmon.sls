{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{%    for segment_info in salt['mdb_greenplum.get_segment_info']() %}
{{        segment_info['datadir'] }}/gpperfmon/conf/gpperfmon.conf:
  file.managed:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0600
    - source: salt://components/greenplum/conf/gpconfigs/gpperfmon.conf
    - template: jinja
    - makedirs: True
    - context:
      content_id: {{ segment_info['content'] }}
      port: {{ segment_info['port'] }}
      datadir: {{ segment_info['datadir'] }}
    - create: False
    - require_in:
{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}
      - cmd: gpperfmon_install
{%        else %}
      - cmd: gp-wait-started
{%        endif %}
{%    endfor %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}
gpperfmon_install:
  cmd.run:
    - name: {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/gpperfmon_install --port {{ gpdbvars.master_port }} --enable --verbose --do-not-edit-configs --role monitor --do-not-create-role
    - cwd: /home/{{ gpdbvars.gpadmin }}
    - runas: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - shell: /bin/bash
    - env:
      - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
      - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
      - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
      - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
      - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
    - unless: >
        {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/psql --dbname postgres -XtAc "SELECT 1 FROM pg_database WHERE datname='gpperfmon'" | grep -q 1
{%    if salt['pillar.get']('service-restart', False) %}
    - require_in:
      - greenplum-stop
{%    endif %}
{% endif %}

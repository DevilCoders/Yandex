{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}

disable_segments_autorecovery:
  file.managed:
    - name: /tmp/.autorecovery_disabled.flag
    - replace: False

{%   if salt['mdb_greenplum.check_segment_replica_status']().get('result', False) %}
gp-cluster-segment-down:
  cmd.run:
    - name: $GPHOME/bin/gpstop -a --host {{ salt.pillar.get('segment_fqdn') }}
    - runas: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - shell: /bin/bash
    - env:
      - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
      - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
      - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
      - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
      - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'

gpdb_check_segment_down:
  mdb_greenplum.check_segment_is_in_dead_status_call:
    - require:
      - cmd: gp-cluster-segment-down
{%   else %}
gpdb_check_segment_down:
  mdb_greenplum.check_segment_is_in_dead_status_call
{%   endif %}
{% else %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}

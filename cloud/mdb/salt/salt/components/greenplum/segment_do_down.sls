{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}

gpdb_check_segments_are_up:
  mdb_greenplum.check_segment_replicas_up_status_call

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
    - require:
      - mdb_greenplum: gpdb_check_segments_are_up

gpdb_check_segment_down:
  mdb_greenplum.check_segment_is_in_dead_status_call:
    - require:
      - cmd: gp-cluster-segment-down

{% else %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}

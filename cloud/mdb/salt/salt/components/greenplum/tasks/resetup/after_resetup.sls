{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}

gpdb_check_segment_is_up:
  mdb_greenplum.check_segment_replica_status_call

gp-cluster-rebalance-segments:
    cmd.run:
        - name: $GPHOME/bin/gprecoverseg -ar
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'

enable_segments_autorecovery:
  file.absent:
    - name: /tmp/.autorecovery_disabled.flag
    - require:
      - mdb_greenplum: gpdb_check_segment_is_up
      - cmd: gp-cluster-rebalance-segments
{% else %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}

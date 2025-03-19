{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}  

gpdb_check_replica_master_is_up:
  mdb_greenplum.check_master_replica_is_alive_call

greenplum-clear-back:
  cmd.run:
    - name: rm -rf {{ gpdbvars.masterdir }}/master/back-data-{{ gpdbvars.segprefix }}-1
    - require:
      - mdb_greenplum: gpdb_check_replica_master_is_up

service_disable:
  service.running:
    - name: greenplum
    - enable: False
    - require:
      - mdb_greenplum: gpdb_check_replica_master_is_up

greenplum-stop-master:
  cmd.run:
    - name: $GPHOME/bin/gpstop -a -M fast -l {{ gpdbvars.gplog }}
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
      - service: service_disable
      - cmd: greenplum-clear-back

move-to-back:
  cmd.run:
    - name: mv {{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1 {{ gpdbvars.masterdir }}/master/back-data-{{ gpdbvars.segprefix }}-1
    - require:
      - cmd: greenplum-stop-master

{% else %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}
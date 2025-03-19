{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}
/home/gpadmin/.ssh/known_hosts:
  file.absent

gp-cluster-ssh_avail:
    cmd.run:
        - name: /usr/local/yandex/gp_wait_cluster.py -w 1 -r 600 -s
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
          - file: /home/gpadmin/.ssh/known_hosts

gp-cluster-recover-segments:
    cmd.run:
        - name: $GPHOME/bin/gprecoverseg -a || $GPHOME/bin/gprecoverseg -F -a
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
          - cmd: gp-cluster-ssh_avail
          - file: /home/gpadmin/.ssh/known_hosts

gpdb_sync_users:
  mdb_greenplum.check_segment_replica_status_call:
    - require:
      - cmd: gp-cluster-recover-segments

{% else %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}

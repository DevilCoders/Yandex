{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == '' %}  

gp-cluster-recover-master:
    cmd.run:
        - name: $GPHOME/bin/gpactivatestandby -a -f -d $MASTER_DATA_DIRECTORY
        - runas: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - shell: /bin/bash
        - env:
          - PGPORT: 5432
          - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
          - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
          - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
          - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
          - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'

greenplum-stop:
    cmd.run:
        - name: $GPHOME/bin/gpstop -a -M fast
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
          - cmd: gp-cluster-recover-master

greenplum-start:
  service.running:
    - name: greenplum
    - enable: true
    - require:
      - cmd: greenplum-stop

{% elif salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}

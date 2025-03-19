{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}

{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1/pg_hba.conf:
  file.managed:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0644
    - source: salt://components/greenplum/conf/gpconfigs/pg_hba.conf
    - template: jinja

greenplum-reload-master:
  cmd.run:
    - name: $GPHOME/bin/gpstop -a -u
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
      - file: {{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1/pg_hba.conf

{% else %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}
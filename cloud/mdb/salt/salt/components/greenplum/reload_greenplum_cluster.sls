{% from "components/greenplum/map.jinja" import gpdbvars with context %}

reload-greenplum-cluster:
  cmd.run:
    - name: {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/gpstop -uq
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
    - require:
      - file: {{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1/pg_hba.conf
    - watch:
      - file: {{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1/pg_hba.conf

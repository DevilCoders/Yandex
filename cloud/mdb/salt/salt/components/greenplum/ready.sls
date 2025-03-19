{% from "components/greenplum/map.jinja" import gpdbvars with context %}

greenplum-ready:
  test.nop:
    - require:
      - file: greenplum-service
      - file: /root/.pgpass
      - file: /home/{{ gpdbvars.gpadmin }}/.pgpass

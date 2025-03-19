{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}

greenplum-stop:
  service.dead:
    - name: greenplum

greenplum-start:
  service.running:
    - name: greenplum
    - require:
      - greenplum-stop

{% else %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}

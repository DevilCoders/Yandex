{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{% if salt['mdb_greenplum.get_active_master']() == salt.grains.get('id','') %}  

greenplum-stop:
  service.dead:
    - name: greenplum
    - enable: false

{% else %}
greenplum-do_nothing-recover-segments:
    test.nop
{% endif %}

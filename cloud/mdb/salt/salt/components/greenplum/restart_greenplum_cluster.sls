{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{% if salt.grains.get('greenplum:role') == 'master' %}

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

{% from "components/greenplum/map.jinja" import gpdbvars,sysvars,pxfvars with context %}

{% if pxfvars.major_version == 6 and salt['file.directory_exists']( pxfvars.pxfhome + '-5' ) %}
pxf:
  service.dead

/lib/systemd/system/pxf.service:
  file.absent:
    - require:
      - service: pxf
    - onchanges_in:
      - module: systemd-reload

greenplum-pxf-5:
  pkg.purged:
    - require:
      - service: pxf 

remove_pxf_conf_dir:
  cmd.run:
    - name: 'rm -rf /etc/greenplum-pxf'
    - require:
      - pkg: greenplum-pxf-5

{{ pxfvars.pxfhome }}-5:
  file.absent:
    - require:
      - pkg: greenplum-pxf-5
{% else %}
nothing_to_do:
  test.nop
{% endif %}

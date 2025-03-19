{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{% if salt.pillar.get('data:gpbackup_install', False) %}
{%   for util in gpdbvars.gpbackup_utils.split(',') %}
{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/{{ util }}:
  file.managed:
    - source: https://github.com/greenplum-db/gpbackup/releases/download/{{ gpdbvars.gpbver }}/{{ util }}
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0755
    - skip_verify: True
    - require:
      - pkg: {{ gpdbvars.gppkgname }}-{{ gpdbvars.gpmajver }}
      - user : {{ gpdbvars.gpadmin }}
    - unless: "{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/{{ util }} --version | grep {{ gpdbvars.gpbver }} >> /dev/null 2>&1"
{%   endfor %}
{% endif %}

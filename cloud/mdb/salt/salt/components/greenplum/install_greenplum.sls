{% from "components/greenplum/map.jinja" import gpdbvars,sysvars with context %}

{{ gpdbvars.gppkgname }}-{{ gpdbvars.gpmajver }}:
  pkg.installed:
    - version: {{ gpdbvars.gppkgver }}
    - prereq_in:
      - cmd: repositories-ready

{{ gpdbvars.gppkgname }}-{{ gpdbvars.gpmajver }}-dbgsym:
  pkg.installed:
    - version: {{ gpdbvars.gppkgver }}
    - prereq_in:
      - cmd: repositories-ready
    - require:
      - pkg: {{ gpdbvars.gppkgname }}-{{ gpdbvars.gpmajver }}

/usr/local/bin/psql:
  file.symlink:
    - target: {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/psql
    - require:
      - pkg: {{ gpdbvars.gppkgname }}-{{ gpdbvars.gpmajver }}

{{ sysvars.ldconf }}:
  file.append:
    - text:
      - {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/
    - require:
      - pkg: {{ gpdbvars.gppkgname }}-{{ gpdbvars.gpmajver }}

/sbin/ldconfig:
  cmd.run:
    - onchanges:
      - file: {{ sysvars.ldconf }}

/home/{{ gpdbvars.gpadmin }}/gpconfigs:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}

/var/lib/greenplum:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
{% if salt.dbaas.is_compute() and salt['pillar.get']('data:dbaas_compute:vdb_setup', True) %}
    - require_in:
      - cmd: mount-data-directory
{% endif %}

/var/run/greenplum:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}

{{ gpdbvars.gplog }}:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}


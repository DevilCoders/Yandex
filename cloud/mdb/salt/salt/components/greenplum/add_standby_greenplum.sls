{% from "components/greenplum/map.jinja" import gpdbvars with context %}

{% if salt.pillar.get('standby_install', False) %}
{%   if salt.pillar.get('gpdb_master') %}
Add-standby-to-existing-greenplum-cluster:
  cmd.run:
    - name: {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/gpinitstandby -a
          -s {{ salt.pillar.get('standby_master_fqdn') }}
          -l {{ gpdbvars.gplog }}
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
    - onlyif: gpstate -f | grep "Standby master instance not configured"
    - require:
      - sls: components.greenplum.init_greenplum
{%   endif %}
{% endif %}


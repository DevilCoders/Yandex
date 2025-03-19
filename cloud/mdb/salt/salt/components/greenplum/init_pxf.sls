{% from "components/greenplum/map.jinja" import gpdbvars,sysvars,pxfvars with context %}
{% set java_home = salt['cmd.shell']('echo $(dirname $(dirname $(readlink $(readlink $(which javac)))))') %}
{% set conf_var = 'PXF_CONF' if pxfvars.major_version == 5 else 'PXF_BASE' %}
{% set command = 'init' if pxfvars.major_version == 5 else 'prepare' %}

include:
  - components.greenplum.remove_pxf

Init-PXF:
  cmd.run:
    - name: {{ pxfvars.pxfhome }}-{{ pxfvars.major_version }}/bin/pxf {{ command }}
    - cwd: /home/{{ gpdbvars.gpadmin }}
    - runas: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - shell: /bin/bash
    - env:
      - GPHOME: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
      - PYTHONPATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib/python'
      - PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin'
      - LD_LIBRARY_PATH: '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/lib'
      - JAVA_HOME: '{{ java_home }}'
      - MASTER_DATA_DIRECTORY: '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
      - {{ conf_var }}: '{{ pxfvars.pxfconf }}'
{% if pxfvars.major_version == 5 %}
    - onlyif: {{ pxfvars.pxfhome }}-{{ pxfvars.major_version }}/bin/pxf status | fgrep -q 'init'
{% elif pxfvars.major_version == 6 %}
    - creates: {{ pxfvars.pxfconf }}/conf
{% endif %}
    - require:
      - sls: components.greenplum.install_pxf
      - sls: components.greenplum.remove_pxf
      - file: {{ pxfvars.pxfconf }}
      - file: {{ pxfvars.java_io_tmpdir }}

/lib/systemd/system/{{ pxfvars.service_name }}.service:
  file.managed:
    - mode: 0644
    - source: salt://{{ slspath }}/conf/pxf/pxf.service
    - template: jinja
    - require_in:
      - service: {{ pxfvars.service_name }}
    - require:
      - pkg: greenplum-pxf-{{ pxfvars.major_version }}
      - cmd: Init-PXF
    - onchanges_in:
      - module: systemd-reload

pxf.service:
  service.running:
    - name: {{ pxfvars.service_name }}
    - enable: True
    - watch:
      - file: {{ pxfvars.pxfconf }}/servers*
{% if pxfvars.major_version == 6 %}
      - file: {{ pxfvars.pxfconf }}/lib
{% endif %}
      - pkg: greenplum-pxf-{{ pxfvars.major_version }}
    - require:
      - sls: components.greenplum.remove_pxf
      - file: {{ pxfvars.pxfconf }}/conf*

{{ pxfvars.pxfconf }}/conf/pxf-profiles.xml:
  file.managed:
    - mode: 0644
    - source: salt://{{ slspath }}/conf/pxf/pxf-profiles.xml
    - template: jinja
    - require:
      - file: /lib/systemd/system/{{ pxfvars.service_name }}.service

{{ pxfvars.pxfconf }}/conf/pxf-env.sh:
  file.managed:
    - mode: 0644
    - source: salt://{{ slspath }}/conf/pxf/pxf-env{{ pxfvars.major_version }}.sh
    - template: jinja
    - require:
      - file: /lib/systemd/system/{{ pxfvars.service_name }}.service

{% if pxfvars.major_version == 5 %}
{{ pxfvars.pxfconf }}/conf/pxf-log4j.properties:
  file.managed:
    - mode: 0644
    - source: salt://{{ slspath }}/conf/pxf/pxf-log4j.properties
    - template: jinja
    - require:
      - file: /lib/systemd/system/{{ pxfvars.service_name }}.service
{% endif %}

{% if pxfvars.major_version == 6 %}
{{ pxfvars.pxfconf }}/conf/pxf-log4j2.xml:
  file.managed:
    - mode: 0644
    - source: salt://{{ slspath }}/conf/pxf/pxf-log4j2.xml
    - template: jinja
    - require:
      - file: /lib/systemd/system/{{ pxfvars.service_name }}.service

{{ pxfvars.pxfconf }}/conf/pxf-application.properties:
  file.managed:
    - mode: 0644
    - source: salt://{{ slspath }}/conf/pxf/pxf-application.properties
    - template: jinja
    - require:
      - file: /lib/systemd/system/{{ pxfvars.service_name }}.service
      
{{ pxfvars.pxfconf }}/lib:
  file.symlink:
    - target: {{ pxfvars.pxfhome }}-{{ pxfvars.major_version }}/lib
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - force: True
    - require:
      - cmd: Init-PXF
{% endif %}

/var/log/greenplum-pxf:
  file.absent

{% if salt.pillar.get('gpdb_master') or salt.grains.get('greenplum:role') == 'master' %}
{%   set pxf_extension_version = '2.0' if pxfvars.major_version == 6 else '1.0' %}
update_pxf_extension_in_all_dbs:
  mdb_greenplum.extension_update_in_all_dbs:
    - name: pxf
    - version: {{ pxf_extension_version }}
    - require:
      - pkg: greenplum-pxf-{{ pxfvars.major_version }}
      - cmd: Init-PXF
      - sls: components.greenplum.remove_pxf
      - sls: components.greenplum.extensions
{% endif %}

# pxf watchdog
/etc/cron.d/wd-pxf:
  file.managed:
    - source: salt://{{ slspath }}/conf/cron.d/wd-pxf
    - mode: 644
    - user: root
    - group: root
    - require:
      - pxf.service
      - file: /etc/cron.yandex/wd-pxf.sh

/etc/cron.yandex/wd-pxf.sh:
  file.managed:
    - source: salt://{{ slspath }}/conf/pxf/wd-pxf.sh
    - mode: 755
    - user: root
    - group: root
    - template: jinja

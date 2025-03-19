{% from "components/greenplum/map.jinja" import gpdbvars,sysvars,pxfvars with context %}
{% set java_home = salt['cmd.shell']('echo $(dirname $(dirname $(readlink $(readlink $(which javac)))))') %}

{% if salt.pillar.get('data:pxf:install', True) %}
java_pkgs:
  pkg.installed:
    - pkgs:
      - openjdk-11-jdk
      - openjdk-11-jre

greenplum-pxf-{{ pxfvars.major_version }}:
  pkg.installed:
    - version: {{ pxfvars.pkgver }}
    - prereq_in:
      - cmd: repositories-ready
    - require:
      - pkg: java_pkgs

{% set conf_var = 'PXF_CONF' if pxfvars.major_version == 5 else 'PXF_BASE' %}
Add-java-home-to-.bashrc:
  file.accumulated:
    - filename: /home/{{ gpdbvars.gpadmin }}/.bashrc
    - text:
      - "export JAVA_HOME={{ java_home }}"
      - "export PATH=$PATH:{{ pxfvars.pxfhome }}-{{ pxfvars.major_version }}/bin"
      - "export {{ conf_var }}={{ pxfvars.pxfconf }}"
    - require_in:
      - file: bashrc-config-block-1

{{ pxfvars.pxfconf }}:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0755

{{ pxfvars.java_io_tmpdir }}:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0755
    - makedirs: True
{% endif %}

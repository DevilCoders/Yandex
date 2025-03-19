{% from "components/greenplum/map.jinja" import dep, gpdbvars, sysvars with context %}

{% set subpkg = salt['pillar.get']('data:versions:greenplum:package_version').split('-') %}
{% set version, patch_level = subpkg[0], subpkg[1] if 'mdb' not in subpkg[1] else subpkg[2] %}
{% set mode = '750' if gpdbvars.version|int == 6175 and gpdbvars.patch_level|int >= 62 or gpdbvars.version|int >= 6193 else '700' %}

bashrc-config-block-1:
  file.blockreplace:
    - name: /home/{{ gpdbvars.gpadmin }}/.bashrc
    - marker_start: "# START Added by SaltStack"
    - marker_end: "# END Added by SaltStack"
    - append_if_not_found: True
    - show_changes: True

bashrc-config-block-1-{{ gpdbvars.masterdir }}:
  file.accumulated:
    - filename: /home/{{ gpdbvars.gpadmin }}/.bashrc
    - text:
      - "export MASTER_DATA_DIRECTORY={{ gpdbvars.masterdir }}/master/gpseg-1"
      - "source {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/greenplum_path.sh"
      - "export LD_PRELOAD={{ dep.libz }} ps"
      - "export PGDATABASE=postgres"
    - require_in:
      - file: bashrc-config-block-1

greenplum-root-bashrc:
  file.accumulated:
    - name: root-bashrc
    - filename: /root/.bashrc
    - text: |-
        # greenplum variables
        export MASTER_DATA_DIRECTORY={{ gpdbvars.masterdir }}/master/gpseg-1
        source {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/greenplum_path.sh
        export LD_PRELOAD={{ dep.libz }} ps
        export PGDATABASE=postgres
        export PGUSER={{ gpdbvars.gpadmin }}
        psql () {
          sudo -u {{ gpdbvars.gpadmin }} {{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}/bin/psql "options='-c log_statement=none -c log_min_messages=panic -c log_min_error_statement=panic -c log_min_duration_statement=-1' dbname=postgres" "${@:1}"
        }
        alias wal-g='wal-g --config /etc/wal-g/wal-g.yaml'
    - require_in:
      - file: /root/.bashrc

{% if not salt['pillar.get']('data:running_on_template_machine', False) %}
{% set segments_info = salt['mdb_greenplum.get_segment_info']() %}

{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
/home/{{ gpdbvars.gpadmin }}/{{ gpdbvars.gpconfdir }}/segments:
  file.managed:
    - source: salt://{{ slspath }}/conf/gpconfigs/segments
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0644
    - template: jinja

/home/{{ gpdbvars.gpadmin }}/{{ gpdbvars.gpconfdir }}/cluster:
  file.managed:
    - source: salt://{{ slspath }}/conf/gpconfigs/cluster
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0644
    - template: jinja

/home/{{ gpdbvars.gpadmin }}/{{ gpdbvars.gpconfdir }}/configuration_file:
  file.managed:
    - source: salt://{{ slspath }}/conf/gpconfigs/configuration_file
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0644
    - template: jinja

/home/{{ gpdbvars.gpadmin }}/{{ gpdbvars.gpconfdir }}/gpinitsystem_config:
  file.managed:
    - source: salt://{{ slspath }}/conf/gpconfigs/gpinitsystem_config
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0644
    - template: jinja

/home/{{ gpdbvars.gpadmin }}/{{ gpdbvars.gpconfdir }}/gpopts.conf:
  file.managed:
    - source: salt://{{ slspath }}/conf/gpconfigs/gpopts.conf
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: 0644
    - template: jinja

master-{{ gpdbvars.masterdir }}:
  file.directory:
    - name: {{ gpdbvars.masterdir }}
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}

{{ gpdbvars.masterdir }}/master:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - makedirs: True

{# MDB-15383: Probably can be removed after GP release "6.17.5-62-yandex.52348.09df96f263" #}
{# Need only for clusters created before this ticket done #}
{% for segment_info in segments_info %}
{%   if segment_info['hostname'] == salt['grains.get']('fqdn') %}
{%     if salt['file.directory_exists'](segment_info['datadir']) %}
set_permissions_for_{{ segment_info['datadir'] }}:
  file.directory:
    - name: {{ segment_info['datadir'] }}
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: {{ mode }}
{%     endif %}
{%     if salt['file.directory_exists'](segment_info['datadir'] + '/' + 'pg_log') %}
{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1/pg_log:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: {{ mode }}
{%     endif %}
{%   endif %}
{% endfor %}
{% endif %}

{% if salt.pillar.get('data:dbaas:subcluster_name') == 'segment_subcluster' %}
{%   for dir in gpdbvars.data_folders %}
Create-{{ dir }}:
  file.directory:
    - name: {{ dir }}
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - makedirs: True

{{ dir }}/primary:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - makedirs: True

{{ dir }}/mirror:
  file.directory:
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - makedirs: True


{# MDB-15383: Probably can be removed after ticket closed #}
{# Need only for clusters created before this ticket done #}
{%      for segment_info in segments_info %}
{%         if segment_info['hostname'] == salt['grains.get']('fqdn') %}
{% if salt['file.directory_exists'](segment_info['datadir']) %}
set_permissions_for_{{ segment_info['datadir'] }}:
  file.directory:
    - name: {{ segment_info['datadir'] }}
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: {{ mode }}
{% endif %}

{% if salt['file.directory_exists' ]( segment_info['datadir'] + '/' + 'pg_log') %}
{{ segment_info['datadir'] }}/pg_log:
  file.directory:
    - name: {{ segment_info['datadir'] }}/pg_log
    - user: {{ gpdbvars.gpadmin }}
    - group: {{ gpdbvars.gpadmin }}
    - mode: {{ mode }}
{% endif %}
{%          endif %}
{%      endfor %}
{%   endfor %}
{% endif %}

{% endif %}
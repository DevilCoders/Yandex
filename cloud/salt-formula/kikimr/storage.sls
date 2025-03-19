{%- import "common/kikimr/init.sls" as vars with context %}

{% set wait_timeout = 0 if opts['test'] else 6000 %}

{% set cluster_id = vars.kikimr_config['cluster_id'] %}
{% set kikimr_cluster = grains['cluster_map']['kikimr']['clusters'][cluster_id] %}

{% set storage_nodes = kikimr_cluster['storage_nodes'] %}
{% set first_storage_node = storage_nodes|sort|first %}
{% set kikimr_path = "/Berkanavt/kikimr" %}

{% set grpc_port = 2135 %}
{% set mon_port = 8765 %}
{% set ic_port = 19001 %}

{% if vars.use_config_packages %}
remove_old_kikimr_storage_configs:
  file.absent:
    - name: /Berkanavt/kikimr/cfg
    - onlyif:
      - test -L /Berkanavt/kikimr/cfg

storage_kikimr_cfg_packages:
  yc_pkg.installed:
    - pkgs:
      - yandex-search-kikimr-storage-conf-{{ vars.environment }}-{{ cluster_id }}
    - disable_update: True
    - require:
      - file: remove_old_kikimr_storage_configs
    - require_in:
      - service: kikimr_env_generator
      - service: kikimr
    - watch_in:
      - service: kikimr
{% endif %}

# CLOUD-15753: formating kikimr disks leave only for CloudVM,
# For HW it was moved in https://bb.yandex-team.ru/projects/CLOUD/repos/yc-setup
# and will execute during deploy host
{% if vars.base_role == 'cloudvm' %}
kikimr_stop:
  service.dead:
    - names:
      - kikimr
    - unless: test -e /Berkanavt/kikimr/file.data.formated
    - watch:
      - yc_pkg: kikimr_packages

format_disks:
  file.managed:
    - name: /Berkanavt/kikimr/scripts/format_drives.bash
    - makedirs: True
    - source: salt://{{ slspath }}/files/format_drives.bash
    - template: jinja
    - require:
      - yc_pkg: kikimr_packages
      - service: kikimr_stop
  cmd.run:
    - name: /bin/bash scripts/format_drives.bash
    - cwd: /Berkanavt/kikimr
    - require:
      - file: format_disks
    - onchanges:
      - file: format_disks
    - unless: test -e /Berkanavt/kikimr/file.data.formated

mark_disk_formated:
  cmd.run:
    - name: touch /Berkanavt/kikimr/file.data.formated
    - require:
      - cmd: format_disks
    - unless: test -e /Berkanavt/kikimr/file.data.formated
{% endif %}

kikimr_env_generator:
  service.running:
    - name: kikimr-autoconf
    - enable: True

kikimr:
  service.running:
    - enable: True
    - watch_in:
      - http: wake_up_kikimr
    - require:
      - service: kikimr_env_generator

wake_up_kikimr:
  http.wait_for_successful_query:
    - name: "{{ vars.proto_prefix }}://{{ vars.hostname }}:8765/viewer/json/tabletinfo?filter=(Type=BSController)&group=State&enums=true"
    - match: Active
    - wait_for: {{ wait_timeout }}
    - verify_ssl: False
    - request_interval: 3
    - require:
      - service: kikimr

{% if vars.hostname == first_storage_node %}
provision_kikimr:
  cmd.run:
    - names: 
      - /bin/bash cfg/init_storage.bash
      - /bin/bash cfg/init_compute.bash
      - /bin/bash cfg/init_databases.bash
    - cwd: /Berkanavt/kikimr
    - require:
      - service: kikimr
      - http: wake_up_kikimr
    - watch_in:
      - http: check_init
    - unless:
      - test -e /Berkanavt/kikimr/kikimr_provisioned

mark_kikimr_provisioned:
  cmd.run:
    - name: touch /Berkanavt/kikimr/kikimr_provisioned
    - require:
      - cmd: provision_kikimr
    - unless: test -e /Berkanavt/kikimr/kikimr_provisioned
{% endif %}

check_init:
  http.wait_for_successful_query:
    - name: "{{ vars.proto_prefix }}://{{ vars.hostname }}:8765/viewer/json/describe?path=/{{ vars.ydb_domain }}"
    - match: '"CreateFinished":true'
    - verify_ssl: False
    - wait_for: {{ wait_timeout }}
    - request_interval: 3
    - text: True
    - require:
      - http: wake_up_kikimr

{% set is_nbs = True %}
{%- import "common/kikimr/init.sls" as vars with context %}

{% set wait_timeout = 0 if opts['test'] else 6000 %}

{% set nbs_cluster_id = vars.kikimr_config['nbs_cluster_id'] %}
{% set nbs_kikimr_cluster = grains['cluster_map']['kikimr']['clusters'][nbs_cluster_id] %}

{% if nbs_cluster_id or salt['grains.get']('overrides:nbs_endpoint') != '' %}
{% if salt['grains.get']('overrides:nbs_endpoint') == '' %}
{% set storage_nodes = nbs_kikimr_cluster['storage_nodes'] %}
{% set indication_storage_node = storage_nodes|random %}
{% else %}
{% set nbs_config_template_file = 'config_external_cluster.yaml' %}
{% set nbs_cluster_id = nbs_cluster_id if nbs_cluster_id else 'fake_id' %}
{% set indication_storage_node = salt['grains.get']('overrides:nbs_endpoint') %}
{% endif %}

{% if vars.environment in ["hw-ci", "pre-prod", "prod"] %}
remove_old_nbs_configs:
  file.absent:
    - name: /Berkanavt/nbs-server/cfg
    - onlyif:
      - test -L /Berkanavt/nbs-server/cfg

tenant_kikimr_cfg_packages:
  yc_pkg.installed:
    - pkgs:
      - yandex-search-kikimr-nbs-conf-{{ vars.environment }}-{{ nbs_cluster_id }}
    - disable_update: True
    - require:
      - file: remove_old_nbs_configs
    - require_in:
      - service: nbs
    - watch_in:
      - service: nbs
{% endif %}

nbs_server_packages:
  yc_pkg.installed:
    - pkgs:
      - yc-nbs-systemd
      - yandex-cloud-blockstore-server
      - yandex-search-kikimr-kikimr-configure-bin
      - yandex-search-kikimr-kikimr-bin
      - yandex-search-kikimr-python-lib
    - disable_update: True

{% if nbs_config_template_file is defined %}
kikimr_nbs_config:
  file.managed:
    - name: /Berkanavt/nbs-server/cfg/config_cluster_template.yaml
    - source: salt://kikimr/files/{{ nbs_config_template_file }}
    - template: jinja
    - makedirs: True
    - defaults:
        cluster_id: {{ nbs_cluster_id }}
        ydb_domain: {{ vars.ydb_domain }}
    - require:
      - yc_pkg: nbs_server_packages
    - unless:
      - test -e /Berkanavt/nbs-server/cfg/config_generated
  cmd.run:
    - names:
      - ../kikimr/bin/kikimr_configure cfg cfg/config_cluster_template.yaml ../kikimr/bin/kikimr cfg/ --nbs
    - cwd: /Berkanavt/nbs-server
    - onchanges:
      - file: kikimr_nbs_config
    - require:
      - file: kikimr_nbs_config


kikimr_nbs_config_generated:
  file.managed:
    - name: /Berkanavt/nbs-server/cfg/config_generated
    - contents: {{ None|strftime("%B %d %Y %H:%M") }}
    - require:
      - kikimr_nbs_config
    - onchanges:
      - kikimr_nbs_config
{% endif %}

{% endif %}

check_init_nbs_subdomain:
  http.wait_for_successful_query:
    - name: "{{ vars.proto_prefix }}://{{ indication_storage_node }}:8765/viewer/json/describe?path=/{{ vars.ydb_domain }}/{{ vars.nbs_database }}"
    - match: '"CreateFinished":true'
    - verify_ssl: False
    - wait_for: {{ wait_timeout }}
    - request_interval: 3
    - text: True

nbs:
  service.running:
    - enable: True
    - watch:
      - yc_pkg: nbs_server_packages
      - http: check_init_nbs_subdomain

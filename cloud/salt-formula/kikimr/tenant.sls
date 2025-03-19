{%- import "common/kikimr/init.sls" as vars with context %}

{% set wait_timeout = 0 if opts['test'] else 6000 %}

{% set cluster_id = vars.kikimr_config['cluster_id'] %}
{% set kikimr_cluster = grains['cluster_map']['kikimr']['clusters'][cluster_id] %}

{% set storage_nodes = kikimr_cluster['storage_nodes'] %}
{% set first_storage_node = storage_nodes|sort|first %}

{% set databases = vars.kikimr_config['tenant'] %}
{% set kikimr_base_path = "/Berkanavt/kikimr" %}

{% set kikimr_memory_limit_gb = grains['mem_total'] / 1024 / 2 %}

{% set have_secrets_disk = vars.base_role != 'seed' %}

{# should run only in testing! See https://st.yandex-team.ru/CLOUD-20167#5d0cb7a13a4663001f2b9f71 for details #}
{% if vars.environment == "testing" %}
datacenter_file:
  file.managed:
    - name: /etc/yc/kikimr/datacenter.name
    - contents: "{{ grains['cluster_map']['hosts'][vars.hostname]['location']['datacenter'] }}"
    - require_in:
      - service: {{ kikimr_service }}
{% endif %}

{% if vars.need_encryption %}
  {% set secrets_dir = pillar['kikimr_secrets']['secrets_dir'] %}
  {% set secrets_cfg_dir = pillar['kikimr_secrets']['cfg_dir'] %}

kikimr_secrets_dir:
  file.directory:
    - makedirs: True
    - name: {{ secrets_dir }}
  {%- if not have_secrets_disk %}
    - user: root
    - group: kikimr
    - mode: '0710'
    - require:
      - yc_pkg: kikimr_packages
  {%- endif %}

kikimr_secrets_cfg_dir:
  file.directory:
    - makedirs: True
    - name: {{ secrets_cfg_dir }}
    - user: root
    - group: root
    - mode: '0700'
    - require:
      - yc_pkg: kikimr_packages

  {%- if have_secrets_disk %}
    {# Looks lame. We could accidentally overwrite Jinja vars. Use macros or establish vars naming rules! #}
    {% set device_serial = pillar['kikimr_secrets']['disk']['device_serial'] %}
    {% set dm_name = pillar['kikimr_secrets']['disk']['dm_name'] %}
    {% set cfg_dir = pillar['kikimr_secrets']['disk']['cfg_dir'] %}
    {% set mount_where = secrets_dir %}
    {%- include slspath+"/secrets_disk.sls" %}
  {%- endif %}

{% endif %}

{% if vars.base_role == 'seed' or not vars.use_config_packages %}
tenant_monitoring:
  file.managed:
    - name: /home/monitor/agents/etc/kikimr_tenants.conf
    - source: salt://{{ slspath }}/files/monitoring_kikimr_tenants.conf
    - template: jinja
    - defaults:
        databases: {{ databases }}
{% else %}
tenant_monitoring:
  file.absent:
    - name: /home/monitor/agents/etc/kikimr_tenants.conf
{% endif %}

{% for database in databases %}
  {% if databases|length == 1 and vars.use_config_packages %}
    {% set kikimr_service = "kikimr" %}
    {% set kikimr_autoconf_service = "kikimr-autoconf" %}

    {% set kikimr_config_type = "main" %}
    {% if database == vars.sqs_database %}
      {% set kikimr_config_type = "sqs" %}
    {% elif database == vars.s3proxy_database %}
      {% set kikimr_config_type = "s3" %}
    {% endif %}

tenant_kikimr_cfg_packages:
  yc_pkg.installed:
    - pkgs:
      - yandex-search-kikimr-{{ kikimr_config_type }}-unit-conf-{{ vars.environment }}-{{ cluster_id }}
    - disable_update: True
    - require:
      - file: kikimr_encryption_config_{{ database }}
    {% if database != vars.sqs_database %}
    - require_in:
      - service: {{ kikimr_autoconf_service }}
      - service: {{ kikimr_service }}
    - watch_in:
      - service: {{ kikimr_service }}
    {% endif %}
  {% else %}
    {% set kikimr_service = "kikimr@" ~ database %}
    {% set kikimr_autoconf_service = "kikimr-autoconf@" ~ database %}
  {% endif %}

  {% set kikimr_tenant_path = kikimr_base_path + "_" + database %}
  {% set kikimr_tenant_ports = pillar['kikimr_tenant_ports'][database] if loop.length > 1 or vars.base_role == 'cloudvm' else {"grpc_port": 2135, "mon_port": 8765, "ic_port": 19001} %}

  {% if vars.need_encryption %}
    {% set tenant_key_id = pillar['kikimr_secrets']['tenants_keys'][database] %}

    {% set encryption_dir = secrets_dir + "/" + database %}
kikimr_secrets_dir_{{ database }}:
  file.directory:
    - name: {{ encryption_dir }}
    {%- if not have_secrets_disk %}
    - user: root
    - group: kikimr
    - mode: '0710'
    {%- endif %}
    - require:
      - kikimr_secrets_dir

    {% set tenant_key_dir = encryption_dir + "/key." + tenant_key_id %}
kikimr_key_dir_{{ database }}:
  file.directory:
    - name: {{ encryption_dir }}
    {%- if not have_secrets_disk %}
    - user: root
    - group: kikimr
    - mode: '0710'
    {%- endif %}
    - require:
      - kikimr_secrets_dir_{{ database }}

  {# !The following file MUST exist either on local FS or on mounted secrets disk before this SLS is processed! #}
  {% set tenant_key = tenant_key_dir + "/key.pem" %}
kikimr_encryption_key_{{ database }}:
  file.managed:
    - name: {{ tenant_key }}
    {%- if not have_secrets_disk %}
    - user: root
    - group: kikimr
    - mode: '0640'
    {%- endif %}
    - replace: False
    - require:
      - kikimr_key_dir_{{ database }}
      - file: kikimr_enc_key_password_{{ database }}

    {% set tenant_secrets_cfg_dir = secrets_cfg_dir + "/" + database %}
kikimr_secrets_cfg_dir_{{ database }}:
  file.directory:
    - name: {{ tenant_secrets_cfg_dir }}
    - user: root
    - group: root
    - mode: '0700'
    - require:
      - kikimr_secrets_cfg_dir

    {# !The following file MUST exist either on local FS or on mounted secrets disk before this SLS is processed! #}
    {% set tenant_enc_key_PIN_file = tenant_secrets_cfg_dir + "/password" %}
kikimr_enc_key_password_{{ database }}:
  file.managed:
    - name: {{ tenant_enc_key_PIN_file }}
    - user: root
    - group: kikimr
    - mode: '0640'
    - replace: False
    - require:
      - kikimr_secrets_cfg_dir_{{ database }}

    {% set tenant_enc_key_PIN = salt["cmd.shell"]("cat " + tenant_enc_key_PIN_file) %}

kikimr_encryption_config_{{ database }}:
  file.managed:
    - name: {{ kikimr_tenant_path }}/secrets/key.txt
    - makedirs: True
    - user: root
    - group: kikimr
    - mode: '0640'
    - source: salt://{{ slspath }}/files/key.txt.tmpl
    - template: jinja
    - defaults:
        key_id: {{ tenant_key_id }}
        key_file: {{ tenant_key }}
        key_PIN: {{ tenant_enc_key_PIN }}
    - watch:
      - file: kikimr_encryption_key_{{ database }}
  {% endif %}

  {%- if database == vars.solomon_database %}
    {% set first_database_node = kikimr_cluster['dynamic_nodes'][database]|sort|first %}
    {%- if first_database_node == vars.hostname %}
provision_solomon_volume:
  cmd.run:
    - names:
      - /bin/bash ./cfg/init_solomon_volumes.bash
    - cwd: {{ kikimr_tenant_path }}
    - require:
      - service: {{ kikimr_service }}
    - onchanges:
      - service: {{ kikimr_service }}
    {%- endif %}
  {%- endif %}

/Berkanavt/kikimr_{{ database }}/bin:
  file.symlink:
    - target: /Berkanavt/kikimr/bin

/Berkanavt/kikimr_{{ database }}/libs:
  file.symlink:
    - target: /Berkanavt/kikimr/libs

check_init_{{ database }}:
  http.wait_for_successful_query:
    - name: "{{ vars.proto_prefix }}://{{ first_storage_node }}:8765/viewer/json/describe?path=/{{ vars.ydb_domain }}/{{ database }}"
    - match: '"CreateFinished":true'
    - verify_ssl: False
    - wait_for: {{ wait_timeout }}
    - request_interval: 3
    - text: True
    - require:
      - yc_pkg: kikimr_packages

  {%- if database != vars.sqs_database or vars.base_role == 'cloudvm' %}
    {%- if grains['virtual'] != 'physical' %}
kikimr_systemd_unit_drop-in_{{ kikimr_service }}:
  file.managed:
    - name: /etc/systemd/system/{{ kikimr_service }}.service.d/10-memory-limit.conf
    - source: salt://{{ slspath }}/files/10-memory-limit.conf
    - template: jinja
    - makedirs: True
    - defaults:
        kikimr_memory_limit_gb: {{ kikimr_memory_limit_gb }}
    - require_in:
      - service: {{ kikimr_service }}
    {% endif %}

{{ kikimr_service }}:
  service.running:
    - restart: True
    - enable: True
    - require:
      - service: {{ kikimr_autoconf_service }}_env_generator
      - http: check_init_{{ database }}
    {%- if vars.need_encryption %}
    - watch:
      - file: kikimr_encryption_config_{{ database }}
    {% endif %}

{{ kikimr_autoconf_service }}_env_generator:
  service.running:
    - name: {{ kikimr_autoconf_service }}
    - enable: True

  {%- endif %}

  {% if vars.use_config_packages and vars.base_role != "seed" %}
stop_old_kikimr_tenant_service:
  service.dead:
    - name: "kikimr@{{ database }}"
    - enable: False
    {% if database != vars.sqs_database %}
    - require_in:
      - service: {{ kikimr_service }}
    {% endif %}

masked_old_kikimr_tenant_service:
  service.masked:
    - name: "kikimr@{{ database }}"
    - require:
      - service: stop_old_kikimr_tenant_service
    {% if database != vars.sqs_database %}
    - require_in:
      - service: {{ kikimr_service }}
    {% endif %}
  {% endif %}

{% endfor %}

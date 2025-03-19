{%- import "common/kikimr/init.sls" as vars with context %}

{% set environment = grains['cluster_map']['environment'] %}

kikimr_config_packages:
  yc_pkg.installed:
    - pkgs:
      - yandex-search-kikimr-kikimr-bin
      - yandex-search-kikimr-kikimr-configure-bin

{% for cluster_id, kikimr_cluster in grains['cluster_map']['kikimr']['clusters'].iteritems() %}

  {% set config_dir = vars.mailbox_path ~ "/" ~ cluster_id %}
  {% set config_template = vars.mailbox_path ~ "/" ~ cluster_id ~ "/config_cluster_template.yaml" %}
  {% set config_package = vars.mailbox_path ~ "/" ~ cluster_id ~ ".tar" %}

  {% set ydb_domain = vars.kikimr_prefix ~ cluster_id %}

  {% set storage_nodes = kikimr_cluster['storage_nodes'] %}
  {% set first_storage_node = storage_nodes|sort|first %}

kikimr_config_template_{{ cluster_id }}:
  file.managed:
    - name: {{ config_template }}
    - source: salt://{{ slspath }}/files/config_cluster_template.yaml
    - template: jinja
    - makedirs: True
    - defaults:
        cluster_id: {{ cluster_id }}
        ydb_domain: {{ ydb_domain }}

kikimr_config_{{ cluster_id }}_storage:
  ydb_cfg.configure:
    - name: storage
    - config_dir: {{ config_dir }}
    - template: {{ config_template }}
    - config_bin: {{ vars.bin_path }}/kikimr_configure
    - kikimr_bin: {{ vars.bin_path }}/kikimr
    - defaults:
        seed: {{ kikimr_cluster['seed'] }}
    - require:
        - file: kikimr_config_template_{{ cluster_id }}
    - require_in:
        - filex: kikimr_config_{{ cluster_id }}

kikimr_config_{{ cluster_id }}_storage_init:
  ydb_cfg.configure:
    - name: storage
    - config_dir: {{ config_dir }}
    - template: {{ config_template }}
    - config_bin: {{ vars.bin_path }}/kikimr_configure
    - kikimr_bin: {{ vars.bin_path }}/kikimr
    - init_flag: True
    - defaults:
        seed: {{ kikimr_cluster['seed'] }}
        dynamic: True
        grpc_endpoint: grpc://localhost:2135
    - require:
        - file: kikimr_config_template_{{ cluster_id }}
    - require_in:
        - filex: kikimr_config_{{ cluster_id }}

{% for database in kikimr_cluster['databases'] %}
  {% if database == vars.nbs_database %}
kikimr_config_{{ cluster_id }}_nbs:
  ydb_cfg.configure:
    - name: nbs
    - config_dir: {{ config_dir }}
    - template: {{ config_template }}
    - config_bin: {{ vars.bin_path }}/kikimr_configure
    - kikimr_bin: {{ vars.bin_path }}/kikimr
    - defaults:
        nbs: True
    - require:
        - file: kikimr_config_template_{{ cluster_id }}
    - require_in:
        - filex: kikimr_config_{{ cluster_id }}
  {% else %}
  
  {% set modes = ('', ) if database not in pillar['kikimr_tenant_ports'] else ('', 'seed') %}
  {% for mode in modes %}
    {% set database_dir = database if not mode else database ~ "_" ~ mode %}
  
kikimr_config_{{ cluster_id }}_{{ database }}_{{ mode }}:
  ydb_cfg.configure:
    - name: {{ database_dir }}
    - config_dir: {{ config_dir }}
    - template: {{ config_template }}
    - config_bin: {{ vars.bin_path }}/kikimr_configure
    - kikimr_bin: {{ vars.bin_path }}/kikimr
    - require:
        - file: kikimr_config_template_{{ cluster_id }}
    - defaults:
        dynamic_node: True
        database: /{{ ydb_domain }}/{{ database }}
        cfg_home: {{ vars.base_path }}_{{ database }}
        {% if mode == 'seed' %}
        {% set database_ports = pillar['kikimr_tenant_ports'][database] %}
        grpc_port: {{ database_ports['grpc_port'] }}
        mon_port: {{ database_ports['mon_port'] }}
        ic_port: {{ database_ports['ic_port'] }}
        {% endif %}
    - require_in:
        - filex: kikimr_config_{{ cluster_id }}

      {% if database == vars.solomon_database %}
kikimr_config_{{ cluster_id }}_{{ database }}_{{ mode }}_solomon:
  ydb_cfg.configure:
    - name: {{ database_dir }}
    - config_dir: {{ config_dir }}
    - template: {{ config_template }}
    - config_bin: {{ vars.bin_path }}/kikimr_configure
    - kikimr_bin: {{ vars.bin_path }}/kikimr
    - init_flag: True
    - defaults:
        seed: {{ kikimr_cluster['seed'] }}
        dynamic: True
        grpc_endpoint: grpc://{{ first_storage_node }}:2135
    - require:
        - file: kikimr_config_template_{{ cluster_id }}
    - require_in:
        - filex: kikimr_config_{{ cluster_id }}
      {% endif %} {# if solomon database #}
          
      {% if vars.need_encryption %}
kikimr_config_{{ cluster_id }}_{{ database }}_{{ mode }}_key:
  file.symlink:
    - name: {{ config_dir }}/{{ database_dir }}/key.txt
    - target: {{ vars.base_path }}_{{ database }}/secrets/key.txt
    - require_in:
        - filex: kikimr_config_{{ cluster_id }} 
      {% endif %}
      
    {% endfor %} {# for mode in modes #}

  {% endif %} {# if not nbs database #}

{% endfor %} 

kikimr_config_{{ cluster_id }}:
  filex.from_command:
    - name: {{ config_package }}
    - cmd: tar cf - config_cluster_template.yaml */
    - cwd: {{ config_dir }}
    - quiet: True

kikimr_config_{{ cluster_id }}_cleanup:
  cmd.run:
    - name: rm -r {{ config_dir }}
    - require:
      - kikimr_config_{{ cluster_id }}

{% endfor %}

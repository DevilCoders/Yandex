{%- from "opencontrail/map.jinja" import oct_conf_servers -%}
{%- set hostname = grains['nodename'] -%}

{%- set hosts_count = oct_conf_servers|length -%}
{%- set current_host_index = oct_conf_servers.index(hostname) -%}
{%- set cron_hours = [] %}
{%- for i in range(0, 24, hosts_count) -%}
  {%- if i + current_host_index < 24 -%}
    {%- do cron_hours.append(i + current_host_index) -%}
  {%- endif -%}
{%- endfor -%}

{%- set default_zone_id = pillar['placement']['dc'] %}
{%- set zone_id = salt['grains.get']('cluster_map:hosts:%s:location:zone_id' % hostname, default_zone_id) %}

/usr/local/bin/provision-external-policies:
  file.managed:
    - source: salt://{{ slspath }}/files/provision-external-policies
    - mode: 755

smtp_relay_periodical_lookup:
  cron.present:
    - identifier: smtp_relay_periodical_lookup
    - name: /usr/local/bin/provision-external-policies --network fip-vnet-public@{{ zone_id }} --network fip-vnet-qrator@{{ zone_id }}
    - user: root
    - hour: {{ cron_hours|join(",") }}  {# Script must not run simultaneously, see CLOUD-9125 #}
    - minute: 30
    - require:
      - file: /usr/local/bin/provision-external-policies

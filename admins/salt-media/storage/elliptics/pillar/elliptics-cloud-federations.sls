include:
  - units.ssl.cloud
  - units.karl.control
  - units.federation

is_cloud: true

#TODO: add federation tag
yasmagentnew:
    RAWSTRINGS:
      - 'PORTO="false"'
    instance-getter:
      # {% for itype in ['mastermindresizer'] %}
      # - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
      # {% endfor %}
      {% for itype in ['mdscloud', 'mdsmastermind', 'karl'] %}
      - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_cloud a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
      {% endfor %}
      {% for itype in ['storagesystem'] %}
      - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_cloud a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
      {% endfor %}
      - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_elliptics-cloud

iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'

mastermind-namespace-space:
  log_level: DEBUG
  service: {{ grains["conductor"]["group"] }}-mm-namespace-space
  max_status: 2
  ns_filter: '*'
  defaults:
    percent_warn: 6
    percent_crit: 4
    time_warn: 14d
    time_crit: 7d
    bytes_warn: 100G
    bytes_crit: 10G

salt_state_command_args: "-w 4 -c 10 -a test=False"

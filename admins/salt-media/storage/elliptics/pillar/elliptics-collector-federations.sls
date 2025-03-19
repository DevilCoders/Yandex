include:
  - units.federation

is_collector: true

#TODO: add federation tag
yasmagentnew:
    RAWSTRINGS:
      - 'PORTO="false"'
    instance-getter:
      {% for itype in ['mmnginx'] %}
      - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
      {% endfor %}
      {% for itype in ['mdscloud', 'mdsmastermind'] %}
      - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_collector a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
      {% endfor %}
      {% for itype in ['storagesystem'] %}
      - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_collector a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
      {% endfor %}

iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'

monitoring:
  network_load:
    time: 60

salt_state_command_args: "-w 4 -c 10 -a test=False"

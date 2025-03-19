include:
  - units.ssl.nscfg
  - units.federation
  - units.resource-provider

is_collector: true

yasmagent:
  instance-getter:
    {% for itype in ['mmnginx'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    {% for itype in ['mdscloud', 'storagesystem', 'mdsmastermind'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_collector a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    {% for itype in ['mdsnscfg'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}

iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'

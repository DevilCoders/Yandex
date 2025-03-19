include:
  - units.new-juggler-client


iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"

yasmagent:
    instance-getter:
      {% for itype in ['resizermdsproxy'] %}
      - echo {{ grains['conductor']['fqdn'] }}@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
      {% endfor %}
      - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_resizer-front

golovan-instance-type: resizermdsproxy

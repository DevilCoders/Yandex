cluster : elliptics_ycloud-storage

include:
  - units.elliptics-storage-files
  - units.oom-check
  - units.ssl.storage
  - units.federation

packages-list:
  - python-concurrent.futures
  - syslog-ng
  - python-dmidecode

yasmagentnew:
    instance-getter:
      {% for itype in ['mdsstorage'] %}
      - echo {{ grains['conductor']['fqdn'] }}:100500@{{itype}} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}
      - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_elliptics-ycloud-storage
    RAWSTRINGS:
      - 'PORTO="false"'

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"

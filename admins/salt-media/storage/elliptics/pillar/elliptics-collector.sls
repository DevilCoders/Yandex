cluster: elliptics-collector

include:
  - units.ssl.nscfg
  - units.federation
  - units.resource-provider

is_collector: true

parsers-bin:
  - elliptics_client_parser.py
  - elliptics_server_parser.pl
  - mm-cache.py
  - couples_state.py
  - s3_cleanup_parser.py

graphite-scripts:
  - /usr/lib/yandex-graphite-checks/enabled/mem_process.sh

yasmagent:
  instance-getter:
    {% if 'elliptics-collector-unstable' in grains['c'] %}

    - echo {{ grains['conductor']['fqdn'] }}:100500@mastermindresizer a_prj_none a_ctype_unstable a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mastermindresizer
    - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_unstable a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_elliptics-collector
    {% for itype in ['mmnginx'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_unstable a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    {% for itype in ['mdscloud', 'storagesystem', 'mdsmastermind'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_collector a_ctype_unstable a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}

    {% else %}

    {% for itype in ['mmnginx'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    {% for itype in ['mdscloud', 'storagesystem', 'mdsmastermind'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_collector a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_elliptics-collector
    {% for itype in ['mdsnscfg'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}

    {% endif %}


iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'

monitoring:
  network_load:
    time: 60

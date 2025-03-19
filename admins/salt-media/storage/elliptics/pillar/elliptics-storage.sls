cluster : elliptics-storage

include:
  - units.disk-logbackup
  - units.elliptics-storage-files
  - units.nginx-storage
  - units.oom-check
  - units.push-client.storage
  - units.spacemimic
  - units.ssl.storage
  - units.karl.storage
  - units.federation
  - units.mediastorage-proxy.storage

parsers-bin:
  - elliptics_server_parser.pl
  - mds_storage_nginx_tskv.py
  - mimic-access.py
  - unistorage_traffic.py

packages-list:
  - python-concurrent.futures
  - syslog-ng
  - python-dmidecode

host_in_macro_check:
  macro:
  - _MDSFRONTSNETS_
  - _C_ELLIPTICS_STORAGE_
  api_type: HBF

yasmagentnew:
    instance-getter:
      - /usr/local/bin/srw_instance_getter.sh {{ grains['yandex-environment'] }} {{ grains['conductor']['root_datacenter'] }}
      {% for itype in ['mdsspacemimic', 'mdsstorage', 'porto', 'ape', 'mdsautoadmin'] %}
      - echo {{ grains['conductor']['fqdn'] }}:100500@{{itype}} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
      {% endfor %}
      - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_elliptics-storage
      {% for itype in ['karl'] %}
      - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{itype}} a_prj_storage a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}
    RAWSTRINGS:
      - 'PORTO="false"'

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"

# IOThreadPool0 - cocaine
mds-coredump-ignore-pattern: 'lldpd|CokemulatorSrw|loggiver|nfsensed|cocaine-runtime|asio|Jni|IOThreadPool0'

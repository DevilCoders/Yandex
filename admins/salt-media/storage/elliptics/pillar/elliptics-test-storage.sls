cluster : elliptics-test-storage

include:
  - units.elliptics-storage-files
  - units.nginx-storage
  - units.oom-check
  - units.push-client.test-storage
  - units.spacemimic
  - units.ssl.storage
  - units.karl.storage
  - units.federation
  - units.mediastorage-proxy.storage

parsers-bin:
  - elliptics_server_parser.pl
  - mds_storage_nginx_tskv.py
  - mimic-access.py

packages-list:
  - syslog-ng
  - python-concurrent.futures
  - python-dmidecode

yasmagentnew:
    instance-getter:
      - /usr/local/bin/srw_instance_getter.sh {{ grains['yandex-environment'] }} {{ grains['conductor']['root_datacenter'] }}
      {% for itype in ['porto', 'ape'] %}
      - echo {{ grains['conductor']['fqdn'] }}:100500@{{itype}} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}
      {% for itype in ['mdsspacemimic', 'mdsstorage', 'karl', 'mdsautoadmin'] %}
      - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{itype}} a_prj_storage a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}
    RAWSTRINGS:
      - 'PORTO="false"'

iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'

cocaine-config:
  lepton:
    address: "https://lepton.mdst.yandex.net/"

# push-client - LBOPS-6329
mds-coredump-ignore-pattern: 'lldpd|CokemulatorSrw|loggiver|nfsensed|cocaine-runtime|asio|Jni|push-client'

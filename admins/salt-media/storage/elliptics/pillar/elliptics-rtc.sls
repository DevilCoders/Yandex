cluster : elliptics-rtc

include:
  - units.spacemimic
  - units.oom-check
  - units.push-client.test-storage
  - units.ssl.storage

parsers-bin:
  - elliptics_server_parser.pl
  - mds_storage_nginx_tskv.py
  - mimic-access.py

packages-list:
  - syslog-ng
  - python-concurrent.futures
  - python-dmidecode

elliptics-test-storage-files-755:
  - /usr/local/bin/g3Xflash
  - /usr/bin/spacemimic2tskv.py

elliptics-test-storage-files-644: []

yasmagentnew:
    instance-getter:
      - /usr/local/bin/srw_instance_getter.sh {{ grains['yandex-environment'] }} {{ grains['conductor']['root_datacenter'] }}
      {% for itype in ['mdsspacemimic', 'mdsstorage', 'porto', 'ape'] %}
      - echo {{ grains['conductor']['fqdn'] }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}
    RAWSTRINGS:
      - 'PORTO="false"'

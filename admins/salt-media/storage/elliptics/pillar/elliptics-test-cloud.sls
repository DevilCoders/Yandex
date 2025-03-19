cluster : elliptics-test-cloud

is_cloud: true

include:
  - units.ssl.nscfg
  - units.karl.control
  - units.federation
  - units.resource-provider

parsers-bin:
  - elliptics_client_parser.py
  - elliptics_server_parser.pl
  - mm-cache.py
  - couples_state.py
  - s3_cleanup_parser.py

yasmagent:
  instance-getter:
    {% for itype in ['s3mdscloud', 'mastermindresizer', 'mdsnscfg', 's3goosemaintenance'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    {% for itype in ['mdscloud', 'mdsmastermind', 'karl'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_cloud a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    {% for itype in ['storagesystem'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_cloud a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}

s3:
  ydb:
    address: ydb-ru-prestable.yandex.net:2135
    db_name: /ru-prestable/s3/test/storage

iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'

mastermind-namespace-space:
  log_level: DEBUG
  service: {{ grains["conductor"]["group"] }}-mm-namespace-space
  max_status: 2
  ns_filter: '*'
  defaults:
    percent_warn: 5
  namespaces:
    avatars-pds-yml:
      ignore: true
    avatars-tours:
      ignore: true
    avatars-ynews:
      ignore: true
    avatars-auto:
      percent_warn: 0.5
      percent_crit: 0.1
      bytes_warn: 10G
      bytes_crit: 1G
    music-blobs:
      ignore: true
    ydf-dev:
      ignore: true
    browser-infra:
      ignore: true
    photo-broken:
      bytes_warn: 2G
      bytes_crit: 1G
    avatars-autoru-all:
      ignore: true
    avatars-autoru:
      ignore: true
    avatars-bklk:
      ignore: true

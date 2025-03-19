cluster : elliptics-s3cleanup

include:
  - units.ssl.cloud
  - units.push-client.s3cleanup
  - units.federation
  - units.s3-dbutils.elliptics-s3cleanup

yasmagent:
  instance-getter:
    {% for itype in ['s3goosemaintenance'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}

parsers-bin:
  - s3_cleanup_parser.py

s3:
  ydb:
    address: ydb-ru.yandex.net:2135
    db_name: /ru/s3/prod/storage

iface_ip_ignored_interfaces: 'lo|docker|dummy|vlan688|vlan788|vlan700'

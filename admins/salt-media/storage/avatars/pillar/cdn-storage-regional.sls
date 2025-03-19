include:
  - units.ssl-cdn

parsers-bin:
  - mds_nginx_tskv.pl
  - mds_nginx_tskv.py
  - elliptics_client_parser.py
  - avatars_nginx_ns_size.py
  - fastcgi2-avatars-mds.py

yasmagent:
  instance-getter:
    {% for itype in ['avatarsmdsproxy'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}


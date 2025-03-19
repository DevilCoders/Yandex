include:
  - units.ssl-testing
  - units.secrets
  - units.nscfg
  - units.statbox-push-client-testing

parsers-bin:
  - mds_nginx_tskv.py
  - elliptics_client_parser.py
  - fastcgi2-avatars-mds.py

mds-logbackup-logs: '/var/log/nginx/tskv.log.*.gz'

yasmagent:
  instance-getter:
    {% for itype in ['avatarsmdsproxy'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}

libmastermind_cache_path: /usr/share/libmastermind/mds.cache

yarl_vars:
  quota_spaces:
    - avatar

nginx-monrun-files: []
cluster : avatars-proxy

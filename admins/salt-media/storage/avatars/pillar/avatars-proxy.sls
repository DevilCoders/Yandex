cluster : avatars-proxy

include:
  - units.nginx
  - units.statbox-push-client
  - units.ssl-proxy

parsers-bin:
  - mds_nginx_tskv.pl
  - elliptics_client_parser.py
  - fastcgi2-avatars-mds.py

dashing-ping_cron: /etc/cron.d/pre_ping

dashing-ping_path: /usr/local/share/dashing-ping/dashing-ping-new.py

dashing-ping_pre: /usr/local/share/dashing-ping/pre_ping.py

yasmagent:
    instance-getter: 'echo echo a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_prj_avatars-mds a_itype_avatarsmdsproxy'

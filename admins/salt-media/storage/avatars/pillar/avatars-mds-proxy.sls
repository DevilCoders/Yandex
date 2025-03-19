cluster : avatars-proxy

include:
  - units.nginx
  - units.statbox-push-client
  - units.ssl-mds-proxy
  - units.secrets
  - units.nscfg

parsers-bin:
  - mds_nginx_tskv.pl
  - elliptics_client_parser.py
  - fastcgi2-avatars-mds.py

dashing-ping_cron: /etc/cron.d/pre_ping

dashing-ping_path: /usr/local/share/dashing-ping/dashing-ping-new.py

dashing-ping_pre: /usr/local/share/dashing-ping/pre_ping.py

walle_enabled: True

yasmagent:
  instance-getter:
    {% for itype in ['avatarsmdsproxy'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_avatars-mds-proxy

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"

libmastermind_cache_path: /usr/share/libmastermind/mds.cache

tls_session_tickets:
  /etc/nginx/ssl/tls:
    avatars.mds.yandex.net.ticket: {{ salt.yav.get('sec-01db09sazycepkv7zk6c27t3qj[0]') | json }}
    avatars.mds.yandex.net.ticket.prev: {{ salt.yav.get('sec-01db09sazycepkv7zk6c27t3qj[1]') | json }}
    avatars.mds.yandex.net.ticket.prevprev: {{ salt.yav.get('sec-01db09sazycepkv7zk6c27t3qj[2]') | json }}

yarl_vars:
  quota_spaces:
    - avatar

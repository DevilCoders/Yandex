
cluster : elliptics-proxy

include:
  - units.nginx-proxy
  - units.push-client.proxy
  - units.rsync
  - units.ssl.proxy
  - units.spacemimic
  - units.logdaemon
  - units.mediastorage-proxy.proxy
  - units.mds-hide
  - units.karl.proxy
  - units.federation
  - units.drooz_nginx_local_conf.proxy
  - units.mulcagate-nginx-secrets

# ====== photo-proxy-test =======

photo-proxy-test-additional_pkgs:
  - yandex-juggler-http-check

packages-list:
  - psmisc
  - util-linux
  - python-boto

tls_session_tickets:
  /etc/nginx/ssl/:
    mds_and_storage.key: {{ salt.yav.get('sec-01db09rz1vxzg33ya762t4e91a[0]') | json }}
    mds_and_storage.key.prev: {{ salt.yav.get('sec-01db09rz1vxzg33ya762t4e91a[1]') | json }}
    mds_and_storage.key.prevprev: {{ salt.yav.get('sec-01db09rz1vxzg33ya762t4e91a[2]') | json }}

parsers-bin:
  - mds_nginx_tskv.pl
  - elliptics_client_parser.py
  - mds_nginx_tskv.py
  - s3_parser.py
  - s3_request_parser.py
  - s3_idm_parser.py

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700|vlan788"

yasmagent:
  instance-getter:
    {% for itype in ['mdsproxy', 's3mdsstat', 's3proxy', 'mdsproxynginx', 'mediastorage', 'storagemulcagate', 's3gooseproxy'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@ape a_prj_elliptics-proxy a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_ape
    - echo {{ grains['conductor']['fqdn'] }}:10010@mdscommon a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_mdscommon a_cgroup_elliptics-proxy
    {% for itype in ['karl'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{itype}} a_prj_proxy a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
    {% endfor %}

walle_enabled: True

libmastermind_cache_path: '/var/cache/mastermind/mediastorage-proxy.cache'

elliptics-cache:
  1001:
    path: '/srv/storage/2/1'
    size: 10737418240
  # cache group for mail
  1002:
    path: '/srv/storage/3/1'
    size: 5368709120

nginx-proxy:
  lua:
    ssl_trusted_certificate: "/etc/nginx/ssl/certuml4.pem"
    ssl_verify_depth: 3

# ======= S3 ======= #
s3:
  errorbooster-topic: "/elliptics-proxy/error-booster"
  notifications-topic: "/elliptics-proxy/s3-api-events"
  ydb:
    address: ydb-ru.yandex.net:2135
    db_name: /ru/s3/prod/storage

  private:
    api_address: "https://s3-idm.mds.yandex.net"

  restart_options:
    S3RESTART_CHECK_PUBLIC_DOMAIN: s3.yandex.net
    S3RESTART_CLOSED_HOST_MAX_RPS: 30
    S3RESTART_HOST_HEADER: s3.mds.yandex.net
    S3RESTART_IGNORE_REQUESTS: "(/ping|/s3-ping|/mds-service/banned_urls|/gate/(dv/)?del/[^[:space:]]+|/delete-[^[:space:]]+)"
    S3RESTART_IGNORE_DIRECT: "(4080|4480)"
    S3RESTART_NGINX_TSKV_LOGS: /var/log/nginx/s3-tskv.log,/var/log/nginx/tskv.log
    S3RESTART_OPEN_HOST_MIN_RPS: 140
    S3RESTART_PING_URL: http://localhost//ping-s3-goose-proxy
    S3RESTART_USE_MAINTENANCE_FILE: "yes"

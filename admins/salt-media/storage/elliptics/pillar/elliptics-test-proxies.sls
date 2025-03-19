
cluster : elliptics-test-proxies

include:
  - units.push-client.test-proxy
  - units.rsync
  - units.ssl.test-proxy
  - units.nginx-proxy
  - units.spacemimic
  - units.logdaemon
  - units.mediastorage-proxy.proxy
  - units.mds-hide
  - units.karl.proxy
  - units.federation
  - units.mulcagate-nginx-secrets
  - units.s3-dbutils.elliptics-test-proxies

# ====== photo-proxy-test =======

photo-proxy-test-additional_pkgs:
  - psmisc
  - util-linux
  - yandex-juggler-http-check
  - yandex-free-space-watchdog-nginx-cache
  - yandex-conf-comm-nginx
  - yandex-nginx-scripts
  - yandex-crossdomain-xml-nginx-conf


packages-list:
  - python-boto

parsers-bin:
  - mds_nginx_tskv.pl
  - elliptics_client_parser.py
  - mds_nginx_tskv.py
  - s3_parser.py
  - s3_request_parser.py
  - s3_cleanup_parser.py
  - s3_idm_parser.py

# ======= libmagick ======= #

magic-files:
  - magic

yasmagent:
  instance-getter:
    {% for itype in ['mdsproxy', 's3mdsstat', 's3proxy', 'mdsproxynginx', 'mediastorage', 'storagemulcagate', 's3gooseproxy'] %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{ itype }}
    {% endfor %}
    - echo {{ grains['conductor']['fqdn'] }}:100500@ape a_prj_elliptics-test-proxies a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_ape
    {% for itype in ['karl'] %}
    - /usr/bin/add_federation_tag.sh {{ grains['conductor']['fqdn'] }}:100500@{{itype}} a_prj_proxy a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
    {% endfor %}

elliptics-cache:
  101:
    path: '/srv/storage/2/1'
    size: 10737418240

iface_ip_ignored_interfaces: "lo|docker|dummy|vlan688|vlan700"

libmastermind_cache_path: '/var/cache/mastermind/mediastorage-proxy.cache'

nginx-proxy:
  lua:
    ssl_trusted_certificate: "/etc/yandex-certs/yandexCAs.pem"
    ssl_verify_depth: 3

# ======= S3 ======= #
s3:
  errorbooster-topic: "/elliptics-test-proxies/error-booster"
  notifications-topic: "/elliptics-test-proxies/s3-api-events"
  ydb:
    address: ydb-ru-prestable.yandex.net:2135
    db_name: /ru-prestable/s3/test/storage

  private:
    api_address: "https://s3-idm.mdst.yandex.net"

  restart_options:
    S3RESTART_CLOSED_HOST_MAX_RPS: 10
    S3RESTART_HOST_HEADER: s3.mdst.yandex.net
    S3RESTART_IGNORE_REQUESTS: "(/ping|/s3-ping|/mds-service/banned_urls|/gate/(dv/)?del/[^[:space:]]+|/delete-[^[:space:]]+)"
    S3RESTART_NGINX_TSKV_LOGS: /var/log/nginx/s3-tskv.log,/var/log/nginx/tskv.log
    S3RESTART_OPEN_HOST_MIN_RPS: 20
    S3RESTART_PING_URL: http://localhost//ping-s3-goose-proxy
    S3RESTART_USE_MAINTENANCE_FILE: "yes"

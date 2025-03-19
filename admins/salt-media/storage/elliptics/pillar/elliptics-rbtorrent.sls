{% set conductor_group = grains['conductor']['group'] %}
{% set storages = salt['conductor']['group2children']('elliptics-rbtorrent') %}
{% set rbtorrent_hosts = salt['conductor']['groups2hosts'](conductor_group) %}
{% set rbtorrent_server_name = 'rbtorrent.mds.yandex.net' %}
{% set zk_flock_hosts = salt['conductor']['groups2hosts']('elliptics-zk')  | map('regex_replace', '$', ':2181') | list %}

rbtorrent:
  server_name: '{{rbtorrent_server_name}}'
  do_cleanup: True
  zk_flock_hosts: {{ zk_flock_hosts }}

rbtorrent_check:
  bucket: 'mds-service'
  key: 'rbtorrent-check'
  md5: e2c5886ab308a6e23a3f1c638ed2be6b

rbtorrent_config:
    ALLOWED_NAMESPACES: ['sandbox-tmp', 'docker-registry', 'repo', 'zen', 'arcadia-review', 'devtools']
    MDS_PROXY_URL: 'http://storage-int.mds.yandex.net'
    S3_PROXY_URL: 'http://s3.mds.yandex.net'
    COCAINE_PROXY_URL: 'http://rbtorrent.ape.yandex.net/57'
    MDS_CALC_HASH_PROTO: 'cocaine_proxy'
    MDS_CALC_HASH_APP_NAME: rbtorrent:88
    MONGO_URI: 'mongodb://mds:{{ salt.yav.get('sec-01cskvnt60em9ah5j7ppm7n19n[mds-test-trash-mongo-password]') }}@sas-75xqt4c0usvyhaue.db.yandex.net:27018,vla-woot4945g4pdwrvt.db.yandex.net:27018,vla-w9yrj04ihnpp8otj.db.yandex.net:27018/?replicaSet=rs01&authSource=rbtorrent&socketTimeoutMS=5000&connectTimeoutMS=10000'
    MONGO_CACHE_COLLECTION_NAME: cache_v3
    MONGO_LOCK_COLLECTION_NAME: locks_v2
    MONGO_STORAGE_COLLECTION_NAME: {{ conductor_group }}
    MONGO_STORAGES_COLLECTIONS: {{ storages | list }}
    SKYBONE_PROXY_TIMEOUT: 20
    SKYBONE_JOB_TIMEOUT: 20
    SKYBONE_PROXY_HOSTS: {{ rbtorrent_hosts | list }}
    SKYBONE_PROXY_APP_NAME: rbtorrent:88
    SKYBONE_PROTO: 'http'
    SKYBONE_PROXY_MIN_SUCCESS: 2
    HTTP_LISTEN_HOST: '::'
    HTTP_LISTEN_PORT: 9999
    ANNOUNCE_LOCK_WAIT_TIMEOUT: 120
    CACHE_VALID_TIME: 7200
    LOCK_POLL_PERIOD: 3
    LOCK_HEARTBEAT_PERIOD: 15
    LOCK_VALID_TIME: 30
    CLEANUP_DO_SQLITE: False
    CLEANUP_DO_MONGO_EXPIRED: True
    CLEANUP_DO_MONGO_DELAYED: True
    CLEANUP_NS_BLACKLIST:
      - docker-registry


certificates:
  source: 'certs'
  contents  :
    allCAs.pem: {{ salt.yav.get('sec-01cskvnt60em9ah5j7ppm7n19n[allCAs.pem]') | json }}
    {{rbtorrent_server_name}}.key: {{ salt.yav.get('sec-01eg5j3v0asmm252221fa64r9d[7F000F44D3C8ED5B8860A189FE0002000F44D3_private_key]') | json }}
    {{rbtorrent_server_name}}.pem: {{ salt.yav.get('sec-01eg5j3v0asmm252221fa64r9d[7F000F44D3C8ED5B8860A189FE0002000F44D3_certificate]') | json }}

  packages: ['nginx']
  services: 'nginx'
  check_pkg: 'config-monrun-cert-check'
  path: "/etc/yandex-certs/"

yasmagent:
    instance-getter:
      {% for itype in ['rbtorrent'] %}
      - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}

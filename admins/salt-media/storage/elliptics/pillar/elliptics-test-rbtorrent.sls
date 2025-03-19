{% set rbtorrent_hosts = salt['conductor']['groups2hosts'](grains['conductor']['group']) %}
{% set rbtorrent_server_name = 'rbtorrent.mdst.yandex.net' %}
{% set zk_flock_hosts = salt['conductor']['groups2hosts']('elliptics-test-zk') | map('regex_replace', '$', ':2181') | list %}


rbtorrent:
  server_name: '{{rbtorrent_server_name}}'
  do_cleanup: True
  zk_flock_hosts: {{ zk_flock_hosts }}

rbtorrent_check:
  bucket: 'storage-admin'
  key: 'rbtorrent-check'
  md5: 92b16bcf654b9e2f994afaf48e932d5b

rbtorrent_config:
    ALLOWED_NAMESPACES: ['sandbox-tmp', 'docker-registry', 'sandbox', 'repo', 'zen', 'arcadia-review', 'devtools']
    MDS_PROXY_URL: 'http://storage-int.mdst.yandex.net'
    S3_PROXY_URL: 'http://s3.mdst.yandex.net'
    COCAINE_PROXY_URL: 'http://rbtorrent__v012.apefront.tst12.ape.yandex.net'
    MONGO_URI: 'mongodb://mds:{{ salt.yav.get('sec-01cskvnt60em9ah5j7ppm7n19n[mds-test-trash-mongo-password]') }}@sas-ryjbdgxc5ehm1edb.db.yandex.net:27018,vla-mqvijevtriwev2l7.db.yandex.net:27018,vla-zh0llivy0rj5jwdl.db.yandex.net:27018/?replicaSet=rs01&authSource=rbtorrent&socketTimeoutMS=5000&connectTimeoutMS=5000'
    MONGO_CACHE_COLLECTION_NAME: cache
    MONGO_LOCK_COLLECTION_NAME: locks
    SKYBONE_PROXY_HOSTS: {{ rbtorrent_hosts | list }}
    SKYBONE_PROXY_APP_NAME: rbtorrent:89
    SKYBONE_PROTO: 'http'
    HTTP_LISTEN_HOST: '::'
    HTTP_LISTEN_PORT: 9999
    SKYBONE_PROXY_MIN_SUCCESS: 1
    CLEANUP_DO_SQLITE: False
    CLEANUP_DO_MONGO_EXPIRED: True
    CLEANUP_DO_MONGO_DELAYED: True
    CLEANUP_NS_BLACKLIST:
      - docker-registry

certificates:
  source: 'certs'
  contents  :
    allCAs.pem: {{ salt.yav.get('sec-01cskvnt60em9ah5j7ppm7n19n[allCAs.pem]') | json }}
    {{rbtorrent_server_name}}.key: {{ salt.yav.get('sec-01eg5j3r0y6rhqpz40tdfas4p2[7F000F44D276F0B290C90A72D40002000F44D2_private_key]') | json }}
    {{rbtorrent_server_name}}.pem: {{ salt.yav.get('sec-01eg5j3r0y6rhqpz40tdfas4p2[7F000F44D276F0B290C90A72D40002000F44D2_certificate]') | json }}

  packages: ['nginx']
  services: 'nginx'
  check_pkg: 'config-monrun-cert-check'
  path: "/etc/yandex-certs/"

yasmagent:
    instance-getter:
      {% for itype in ['rbtorrent'] %}
      - echo {{ grains['conductor']['fqdn'] }}:100500@{{ itype }} a_prj_none a_ctype_{{ grains['yandex-environment'] }} a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_{{itype}}
      {% endfor %}

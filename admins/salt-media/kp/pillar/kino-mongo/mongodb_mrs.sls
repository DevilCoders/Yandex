{% set env = grains['yandex-environment'] %}
{% if env == 'production' %}
  {% set sec_id = 'sec-01d5xbgnm6fc5wz7xkye7z0xdb' %}
{% else %}
  {% set sec_id = 'sec-01d5xbhdg5egathj1sdw37md7r' %}
{% endif %}
cluster: mongodb

mongodb:
  stock: True
  version: '3.4.24'
  keyFile: /etc/mongo.key
  enable_daytime: True
  mongod:
    storageEngine:  wiredTiger
  monrun:
    mongodb_disk_read_disabled: True
    mongodb_disk_write_disabled: True
    activeClients-total: <2500
    connections-available: '>2000'
    connections-current: <14000
    indexes-to-memory-ratio: <0.9
    master-present: '>0'
    replica-lag-common: 60
    replica-lag-hidden: 4000
    resident-to-memory-ratio: <0.9
    write-queue: <50
    rs_secondary_alive_count: 1
    rs_secondary_fresh_count: 1
    rs_secondary_fresh_lag: 15
    asserts-user: '<150'
    {% if 'backup' in salt.grains.get("conductor:group") %}
    mongodb_slow_queries_disabled: True
    {% else %}
    mongodb_slow_queries_count: 1%
    {% endif %}
  grants-pillar: yav_mongo:secrets
  keyFilePillar: {{ salt.yav.get(sec_id + '[key]') | json }}
  mongoMonitorPillar: {{ salt.yav.get(sec_id + '[monitor]') | json }}
  mongorcPillar: {{ salt.yav.get(sec_id + '[mongorc]') | json }}
yav_mongo:
  secrets: {{ salt.yav.get(sec_id + '[grants]') | json }}

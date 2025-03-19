{% set env = grains['yandex-environment'] %}
{% if env == 'production' %}
  {% set sec_id = 'sec-01d5xbwc006n6c27dm4n3b0f25' %}
{% else %}
  {% set sec_id = 'sec-01d5xbvwbd90eh64edwvxdb32x' %}
{% endif %}
cluster: mongodb
mongodb:
  stock: True
  version: '3.4.19'
  server_params:
    - --shardsvr
  keyFile: /etc/mongo.key
  enable_daytime: True
  mongod:
    storageEngine:  wiredTiger
  monrun:
    {% if "backup" not in grains["fqdn"]%}
    indexes-to-memory-ratio: <0.8
    {% endif %}
    {% if 'backup' in salt.grains.get("conductor:group") %}
    mongodb_slow_queries_disabled: True
    {% else %}
    {% if 'tasks' in grains['fqdn'] and grains['yandex-environment'] not in ['production', 'prestable'] %}
    mongodb_slow_queries_ms: 1000
    mongodb_slow_queries_count: 20%
    {% elif grains['yandex-environment'] not in ['production', 'prestable'] %}
    mongodb_slow_queries_count: 20%
    {% elif 'tasks' in grains['fqdn'] %}
    mongodb_slow_queries_count: 10%
    {% else %}
    mongodb_slow_queries_count: 1%
    {% endif %}
    {% endif %}
  grants-pillar: yav_mongo:secrets
  keyFilePillar: {{ salt.yav.get(sec_id + '[key]') | json }}
  mongoMonitorPillar: {{ salt.yav.get(sec_id + '[monitor]') | json }}
  mongorcPillar: {{ salt.yav.get(sec_id + '[mongorc]') | json }}
yav_mongo:
  secrets: {{ salt.yav.get(sec_id + '[grants]') | json }}


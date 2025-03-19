mongodb:
  stock: True
  version: '3.6.3'
  keyFile: /etc/mongo.key
  keyFile_nomanage: True
  grants-pillar: cluster_yav_secrets:mongo_grants
  monrun:
    activeClients-total: <500
    {% if 'backup' in salt.grains.get("conductor:group") %}
    mongodb_slow_queries_disabled: True
    {% else %}
    mongodb_slow_queries_ms: 600
    mongodb_slow_queries_show_collscan: True
    mongodb_slow_queries_count: 10%
    {% endif %}

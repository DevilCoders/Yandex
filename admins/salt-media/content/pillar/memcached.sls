{% set yaenv = grains['yandex-environment'] %}
memcached:
  log: '/var/log/memcached/memcached.log'
  memory: 16240
  port: 11211
  user: 'memcache'
  max_object_size: '20m'
  mcrouter:
    port: 5000
    default_policy: AllAsyncRoute 
    {% if yaenv == 'production' %}
    group: content_kino_back
    {% endif %}

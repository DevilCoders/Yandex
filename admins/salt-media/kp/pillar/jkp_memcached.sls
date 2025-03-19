{% set is_prod = grains["yandex-environment"] in ["production", "prestable"] %}

memcached:
  log: '/var/log/memcached/memcached.log'
  memory: 8192
  port: 11211
  user: 'memcache'
  max_object_size: '20m'
  mcrouter:
    port: 5000
    default_policy: AllAsyncRoute
    {% if is_prod %} # TODO: one diff with testing branch
    group: jkp_back
    {% endif %}

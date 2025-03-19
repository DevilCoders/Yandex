memcached:
  log: '/var/log/memcached/memcached.log'
  memory: 512
  port: 11211
  user: 'memcache'
  max_object_size: '20m'
  mcrouter:
    port: 5000
    default_policy: AllFastestRoute

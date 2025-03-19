memcached:
  log: /var/log/memcached/memcached.log
  memory: 128
  port: 11311
  user: memcache
  maxcon: 1024
  restart: False
  max_object_size: 2m
  mcrouter:
    port: 11211
    run_args: '--num-proxies=8 --fibers-max-pool-size=1024 --keepalive-count=100 --max-client-outstanding-reqs=8192 --proxy-max-inflight-requests=8192 --connection-limit=8192'
    default_policy: AllFastestRoute
  mcrouter_cfgname: mcrouter.conf-kp-dev-trusty

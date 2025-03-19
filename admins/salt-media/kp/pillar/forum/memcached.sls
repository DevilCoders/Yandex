memcached:
  log: '/var/log/memcached.log'
  memory: 100
  port: 11311
  user: 'memcache'
  maxcon: 10240
  mcrouter:
    port: 11211
    run_args: '--num-proxies=8 --fibers-max-pool-size=1024 --keepalive-count=100 --max-client-outstanding-reqs=8192 --proxy-max-inflight-requests=8192 --connection-limit=8192'

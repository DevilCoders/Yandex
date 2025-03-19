{% set yaenv = grains['yandex-environment'] %}

loggiver:
  lookup:
    spath: salt://mobile-api/files/loggiver.pattern

memcached:
  memory_warning: 90
  memory_critical: 95
  log: '/var/log/memcached.log'
  memory: 2048
  port: 11211
  user: 'memcache'
  maxcon: 10240
  mcrouter:
    port: 5000
    run_args: '--num-proxies=8 --fibers-max-pool-size=1024 --keepalive-count=100 --max-client-outstanding-reqs=10240 --proxy-max-inflight-requests=10240 --connection-limit=10240'
    get_policy: FailoverRoute
    default_policy: AllAsyncRoute

push_client:
  clean_push_client_configs: true
  port: 8088
  stats:
    - name: "space01h"
      ident: kp
      transport: ipv6
      sszb: True
      send_delay: 30
    - name: "space01e"
      ident: kp
      transport: ipv6
      sszb: True
      send_delay: 30
  logs:
    - file: php/php.log
    - file: php/kinopoisk-metrics.log
    - file: kino-kp1-health/app/app.log
    - file: kino-kp1-health/app/output/stdout
    - file: kino-kp1-health/app/output/stderr

nginx:
  lookup:
    listens:
      listen:
        ports:
          - '80'
          - '[::]:80'
{% if grains['yandex-environment'] in ["development"] %}
      listen_8080:
        ports:
          - '8080'
          - '[::]:8080'
{% endif %}
    log_params:
      name: 'kinopoisk-tskv-mobile-api-log'
      access_log_name: access.log
      custom_fields:
        - "server_port=$server_port"
        - "http_accept_language=$http_accept_language"
        - "http_device=$http_device"
        - "http_x_timestamp=$http_x_timestamp"
        - "http_x_signature=$http_x_signature"
    confd_params:
      # gzip
      gzip_comp_level: '5'
      gzip_proxied: 'expired no-cache no-store private auth'
      # gzip_types: 'text/plain text/xml application/xml text/css application/x-javascript'
      # gzip_min_length: '600'
      gzip_buffers: '1000 64k'
      # gzip_disable: '"MSIE [1-6]\.(?!.*SV1)"'
      output_buffers: '128 64k'
      sendfile: 'off'
      keepalive_timeout: '75 60'
      real_ip_header:   'X-Forwarded-For'
      request_id_from_header: 'on'
    logrotate:
      frequency: hourly
      rotate: 48
      delaycompress: False
      compress: False

php:
  logrotate:
    rotate: 24

{% if grains["yandex-environment"] in ["production", "prestable"] %}
{% set tvmtool_sec_id = 'sec-01dfbgk5mxwh8qkpzfdc9b95vk' %}
{% set mysql_ping_secret='sec-01dyjd0fer7cjdm4kdmjdhv41r' %}
{% elif grains["yandex-environment"] in ["development"] %}
{% set tvmtool_sec_id = 'sec-01dfbgeabw45f8vnrw7rnhjvtz' %}
{% set mysql_ping_secret='sec-01dyjc1fx7evbrt5zv7er7t3s6' %}
{% else %}
{% set tvmtool_sec_id = 'sec-01dfbgq90z6my2sseqsy9zenv0' %}
{% set mysql_ping_secret='sec-01dyjc1fx7evbrt5zv7er7t3s6' %}
{% endif %}
tvmtool_id: {{ salt.yav.get(tvmtool_sec_id+"[id]") }}
tvmtool_secret: {{ salt.yav.get(tvmtool_sec_id+"[secret]") | json }}

mysql_ping_config: {{ salt.yav.get(mysql_ping_secret)[yaenv] | json }}


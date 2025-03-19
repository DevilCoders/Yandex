{% set is_stable = grains["yandex-environment"] in ["production"] %}
{% set yaenv = grains['yandex-environment'] %}

certificates:
  contents:
    wildcard_kinopoisk_ru_kp_yandex_net.pem: {{salt.yav.get('sec-01ct3e6c36crxpkmnd4d0ww45e[item]' if is_stable else 'sec-01ct3e76qndz8z4rjcq1pab9j7[item]')|json}}
    wildcard_kinopoisk_ru_kp_yandex_net_key.pem: {{salt.yav.get('sec-01ct3e6c36crxpkmnd4d0ww45e[key]' if is_stable else 'sec-01ct3e76qndz8z4rjcq1pab9j7[key]')|json}}

loggiver:
  lookup:
    spath: salt://touch-api/files/loggiver.pattern

memcached:
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
      transport: ipv6
      send_delay: 15
    - name: "space01e"
      transport: ipv6
      send_delay: 15
  logs:
    - file: nginx/access.log
    - file: nginx/touch-api.kinopoisk.ru-access.log
    - file: nginx/touch-api.kinopoisk.ru-error.log
    - file: php/fpm-www.access.log
    - file: php/php.log
    - file: kino-kp1-health/app/app.log
    - file: kino-kp1-health/app/output/stdout
    - file: kino-kp1-health/app/output/stderr

nginx:
  lookup:
    enabled: True
    listens:
      listen_ssl:
        ssl:
          ssl_certificate:     '/etc/nginx/ssl/wildcard_kinopoisk_ru_kp_yandex_net.pem'
          ssl_certificate_key: '/etc/nginx/ssl/wildcard_kinopoisk_ru_kp_yandex_net_key.pem'
    log_params:
      name: 'access-log-kp'
      access_log_name: access.log
      custom_fields:
        - "antirobot_status=$antirobot_status"
    confd_params:
      keepalive_timeout: '75 60'
      ignore_invalid_headers: 'on'
      sendfile: 'off'
      output_buffers: '128 64k'
      gzip_buffers: '1000 64k'
      gzip_comp_level: '5'
      gzip_proxied: 'expired no-cache no-store private auth'
      postpone_output: '1460'
      request_id_from_header: 'on'
    logrotate:
      frequency: hourly
      rotate: 48
      delaycompress: False
      compress: False

tls_tickets:
  keyname: touch-api.kinopoisk.ru.key
  robot:
    srckeydir: robot-media-salt

{% if grains["yandex-environment"] in ["production", "prestable"] %}
{% set tvmtool_sec_id = 'sec-01dfbh9m60mnj1shv3jemhbch5' %}
{% set mysql_ping_secret='sec-01dyjd0fer7cjdm4kdmjdhv41r' %}
{% elif grains["yandex-environment"] in ["development"] %}
{% set tvmtool_sec_id = 'sec-01dfbh76hwjqprj96n7zjj05yt' %}
{% set mysql_ping_secret='sec-01dyjc1fx7evbrt5zv7er7t3s6' %}
{% else %}
{% set tvmtool_sec_id = 'sec-01dfbhcgnm24pqnq7qgn9pt8jd' %}
{% set mysql_ping_secret='sec-01dyjc1fx7evbrt5zv7er7t3s6' %}
{% endif %}
tvmtool_id: {{ salt.yav.get(tvmtool_sec_id+"[id]") }}
tvmtool_secret: {{ salt.yav.get(tvmtool_sec_id+"[secret]") | json }}
mysql_ping_config: {{ salt.yav.get(mysql_ping_secret)[yaenv] | json }}


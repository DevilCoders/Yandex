{% set is_prod = grains["yandex-environment"] in ["production", "prestable"] %}
{% set yaenv = grains['yandex-environment'] %}

loggiver:
  lookup:
    spath: salt://backend/files/loggiver.pattern


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
{% if is_prod %}
    - name: logbroker
      proto: pq
      port: 2135
      tvm_client_id_production: 2015955
      tvm_server_id: 2001059
      tvm_secret_file: .tvm_secret
      tvm_secret: {{salt.yav.get('sec-01dn779402s1gmjkx27yz8axed[secret]')|json}}
      fqdn: logbroker.yandex.net
      logger:
        mode: file
        file: /var/log/statbox/logbroker.log
        telemetry_interval: -1
        remote: 0
      logs:
        - file: php/kinopoisk-awaps-gyt.log
          topic: /kinopoisk/kinopoisk-awaps-log
        - file: php/kinopoisk-bunker-gyt.log
          topic: /kinopoisk/kinopoisk-bunker-log
    - name: logbroker-kp-backend
      proto: pq
      port: 2135
      tvm_client_id_production: 2015955
      tvm_server_id: 2001059
      tvm_secret_file: .tvm_secret
      tvm_secret: {{salt.yav.get('sec-01dn779402s1gmjkx27yz8axed[secret]')|json}}
      fqdn: logbroker.yandex.net
      logger:
        mode: file
        file: /var/log/statbox/logbroker-kp-backend.log
        telemetry_interval: -1
        remote: 0
      logs:
        - file: php/kinopoisk-search.log
          topic: /kp-backend/kinopoisk-search-log
    - name: logbroker-kp-master
      proto: pq
      port: 2135
      tvm_client_id_production: 2015955
      tvm_server_id: 2001059
      tvm_secret_file: .tvm_secret
      tvm_secret: {{salt.yav.get('sec-01dn779402s1gmjkx27yz8axed[secret]')|json}}
      fqdn: logbroker.yandex.net
      logger:
        mode: file
        file: /var/log/statbox/logbroker-kp-master.log
        telemetry_interval: -1
        remote: 0
      logs:
        - file: php/kinopoisk-vote-gyt.log
          topic: /kp-master/kinopoisk-review-vote-log
    - name: logbroker-nginx
      proto: pq
      port: 2135
      tvm_client_id_production: 2015955
      tvm_server_id: 2001059
      tvm_secret_file: .tvm_secret
      tvm_secret: {{salt.yav.get('sec-01dn779402s1gmjkx27yz8axed[secret]')|json}}
      fqdn: logbroker.yandex.net
      logger:
        mode: file
        file: /var/log/statbox/logbroker-kp-nginx.log
        telemetry_interval: -1
        remote: 0
      logs:
        - file: nginx/access.log
          topic: /kinopoisk/logs/kp-backend-log
{% endif %}
  logs:
    - file: php/php.log
    - file: php/kinopoisk-metrics.log
    - file: kino-kp1-health/app/app.log
    - file: kino-kp1-health/app/output/stdout
    - file: kino-kp1-health/app/output/stderr
    - file: php/kinopoisk-search.log
    - file: php/kinopoisk-vote-gyt.log
    - file: php/php-hs.log
    - file: php/php-main.log
    - file: php/php-db.log
    - file: nginx/access.log

nginx:
  lookup:
    enabled: True
    vhost_default: false
    listens:
      listen:
        ports:
          - '80'
          - '[::]:80'
{% if "development" in salt["grains.get"]("yandex-environment") %}
      listen_8080:
        ports:
          - '8080'
          - '[::]:8080'
{% endif %}
    log_params:
      name: 'kinopoisk-tskv-backend-log'
      access_log_name: access.log
      custom_fields:
        - "server_port=$server_port"
        - "http_accept_language=$http_accept_language"
        - "http_device=$http_device"
        - "http_x_timestamp=$http_x_timestamp"
        - "http_x_signature=$http_x_signature"
        - "http_x_yandex_icookie=$http_x_yandex_icookie"
    confd_params:
      proxy_buffers: '8000 16k'
      proxy_read_timeout: 600
      proxy_connect_timeout: 300ms

      request_id_from_header: 'on'

      # Cache configuration
      # open_file_cache: 'max=10000 inactive=60s' # - do we need this?
      # open_file_cache_min_uses: '2'             # - there are no much open files on front.
      # open_file_cache_errors: 'on'              # - it is basically for rewrite and proxypass.
      # proxy_cache_path: '/home/.cache levels=1:2 keys_zone=static:60m inactive=10m max_size=2000m'
      # proxy_temp_path: '/home/tmp'
      # proxy_cache_use_stale: 'updating'

      client_header_timeout: '10m'
      client_body_timeout: '10m'
      send_timeout: '10s'
      client_max_body_size: '700m'
      # connection_pool_size: '2048' # do we need this?
      large_client_header_buffers: '1000 8k'
      log_not_found: 'off'
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
      # 00-geobase
      # geobase_path: '/var/cache/geobase/geodata5.bin'
      # limit_req_zone:
      #   - '$binary_remote_addr zone=idx:64m rate=200000r/s'
      #   - '$binary_remote_addr zone=search:64m rate=600000r/s'
      #   - '$binary_remote_addr zone=ratingbtn:128m rate=50r/s'
      #   - '$binary_remote_addr zone=xmlrating:64m rate=20000r/s'
      # limit_req: 'zone=idx burst=400'
      # set_real_ip_from:
      #   - '127.0.0.1'
      #   - '::1'
      real_ip_header:   'X-Forwarded-For'
      # 99-includes'
      # 'include':
        # - 'include/global-maps.conf'
        # - 'environments/upstreams.conf'
        # - 'environments/map-url.conf'

    logrotate:
      frequency: hourly
      rotate: 48
      delaycompress: False
      compress: False

{% if is_prod %}
www_data:
  id_rsa_pub: {{salt.yav.get("sec-01ct3e5g1mk3dgd87tcxkrx1rs[item]")|json}}
  id_rsa: {{salt.yav.get("sec-01ct3e5etxya2h9vwxpdapcj8d[item]")|json}}
s3config-graphdata_secret: {{ salt.yav.get('sec-01dd89p2yfs4shdwcf5tj0dy3d[item]') | json }}
s3config-static_secret: {{ salt.yav.get('sec-01dk9e1bfh44sd8tedk6ses564[item]') | json }}
{% set mysql_ping_secret='sec-01dyjd0fer7cjdm4kdmjdhv41r' %}
{% else %}
s3config-static_secret: {{ salt.yav.get('sec-01drjyghfx3497pn765v1r7h3z[item]') | json }}
{% set mysql_ping_secret='sec-01dyjc1fx7evbrt5zv7er7t3s6' %}
{% endif %}

graphite-sender:
  sender:
    max_metrics: 10000

{% if grains["yandex-environment"] in ["production", "prestable"] %}
{% set tvmtool_sec_id = 'sec-01dfbgxmmz0n3pw6mpa6yr68gc' %}
{% elif grains["yandex-environment"] in ["development"] %}
{% set tvmtool_sec_id = 'sec-01dfbgthf0f614c38dw9ebzdxn' %}
{% else %}
{% set tvmtool_sec_id = 'sec-01dfbh0h9jqatma82mkmkd9tf4' %}
{% endif %}
tvmtool_id: {{ salt.yav.get(tvmtool_sec_id+"[id]") }}
tvmtool_secret: {{ salt.yav.get(tvmtool_sec_id+"[secret]") | json }}

mysql_ping_config: {{ salt.yav.get(mysql_ping_secret)[yaenv] | json }}

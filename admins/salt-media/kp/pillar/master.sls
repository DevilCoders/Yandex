{% set is_stable = grains["yandex-environment"] == "production" %}
{% set yaenv = grains['yandex-environment'] %}

certificates:
  contents:
    bo.kinopoisk.ru.pem: {{salt.yav.get('sec-01ct3e66x7sn0fq58ra0ftqev1[item]' if is_stable else 'sec-01ct3e73x5c5xbdh9pwqyc320n[item]')|json}}
    bo.kp.yandex.net.pem: {{salt.yav.get('sec-01fxfqhycp42x4c3n36g08pgb8[item]' if is_stable else 'sec-01fxgnjdz59370wg5fgbs4f74z[item]')|json}}

loggiver:
  lookup:
    spath: salt://master/files/loggiver.pattern

push_client:
  clean_push_client_configs: true
  ignore_pkgver: true
  stats:
{% if grains['yandex-environment'] in ['prestable', 'production'] %}
    - name: logbroker
      proto: pq
      port: 2135
      tvm_client_id_production: 2015955
      tvm_server_id: 2001059
      tvm_secret_file: .tvm_secret
      tvm_secret: {{salt.yav.get('sec-01dn779402s1gmjkx27yz8axed[secret]')|json}}
      fqdn: logbroker.yandex.net
      server_lite: False
      sszb: False
      logger:
        mode: file
        file: /var/log/statbox/logbroker.log
        telemetry_interval: -1
        remote: 0
      logs:
        - file: php/kinopoisk-parser.log
          sid: "{random}"
          topic: /kp-master/kinopoisk-imdb_parser-log
{% endif %}
  logs:
    - file: php/php.log
    - file: nginx/www.kinopoisk.ru-access.log
    - file: nginx/bo.kinopoisk.ru-access.log
    - file: nginx/access.log
    - file: nginx/error.log
    - file: kino-kp1-health/app/app.log
    - file: kino-kp1-health/app/output/stdout
    - file: kino-kp1-health/app/output/stderr
    - file: php/kinopoisk-parser.log


packages:
  project:
    - nginx-common
    - nginx-full


nginx:
  lookup:
    enabled: True
    listens:
      listen:
        ports:
          - '80'
          - '[::]:80'
      listen_8080:
        ports:
          - '8080'
          - '[::]:8080'
      listen_ssl:
        ports:
          - '443'
          - '[::]:443'
        ssl:
          ssl_certificate:     '/etc/nginx/ssl/bo.kp.yandex.net.pem'
          ssl_certificate_key: '/etc/nginx/ssl/bo.kp.yandex.net.pem'
          ssl_dhparam: '/etc/nginx/ssl/dhparam.pem'
          ssl_ciphers: 'kEECDH+AESGCM+AES128:kEECDH+AES128:kRSA+AESGCM+AES128:kRSA+AES128:DES-CBC3-SHA:!RC4:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2'
          ssl_protocols: 'TLSv1 TLSv1.1 TLSv1.2'
          ssl_session_timeout: '28h'
          ssl_session_cache: 'shared:ssl:128m'
          ssl_prefer_server_ciphers: 'on'
      listen_ssl_bo:
        ports:
          - '443'
          - '[::]:443'
        ssl:
          ssl_certificate:     '/etc/nginx/ssl/bo.kinopoisk.ru.pem'
          ssl_certificate_key: '/etc/nginx/ssl/bo.kinopoisk.ru.pem'
          ssl_dhparam: '/etc/nginx/ssl/dhparam.pem'
          ssl_ciphers: 'kEECDH+AESGCM+AES128:kEECDH+AES128:kRSA+AESGCM+AES128:kRSA+AES128:DES-CBC3-SHA:!RC4:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2'
          ssl_protocols: 'TLSv1 TLSv1.1 TLSv1.2'
          ssl_session_timeout: '28h'
          ssl_session_cache: 'shared:ssl_bo:128m'
          ssl_prefer_server_ciphers: 'on'
      listen_ssl_idm:
        ports:
          - '8443'
          - '[::]:8443'
        ssl:
          ssl_certificate:      '/etc/nginx/ssl/bo.kinopoisk.ru.pem'
          ssl_certificate_key:  '/etc/nginx/ssl/bo.kinopoisk.ru.pem'
          ssl_client_certificate: '/etc/nginx/ssl/allCAs.pem'
          ssl_crl:              '/etc/nginx/ssl/combinedcrl'
          ### KPDUTY-2313
          ssl_verify_client: 'optional'
          ###
          ssl_verify_depth: 3
    log_params:
      name: 'kp-master-log'
      access_log_name: access.log
      custom_fields:
        - "antirobot_status=$antirobot_status"
        - "kp_user_id=$http_x_kp_user_id"
    confd_params:
      keepalive_timeout: '75 60'
      large_client_header_buffers: '1000 8k'
      client_max_body_size: '4000m'
      client_header_timeout: '10m'
      client_body_timeout: '10m'
      send_timeout: '10m'
      sendfile: 'off'
      proxy_buffers: '8000 16k'
      proxy_read_timeout: 3600
      output_buffers: '128 64k'
      gzip_buffers: '1000 64k'
      gzip_comp_level: '5'
      gzip_proxied: 'expired no-cache no-store private auth'
      request_id_from_header: 'on'
      proxy_headers_hash_max_size: '512'
      proxy_headers_hash_bucket_size: '128'

{% if is_stable %}
www_data:
  id_rsa: {{salt.yav.get("sec-01ct3e5yvdng7r5wqxf7p1v0ad[item]")|json}}
rsyncd:
  secrets:
    master: {{salt.yav.get('sec-01ct3e5jp5dw7mjfdb9m8md0j5[master]')|json}}
    static: {{salt.yav.get('sec-01ct3e5jp5dw7mjfdb9m8md0j5[static]')|json}}
kp-migrations:
  check_conf: {{ salt.yav.get('sec-01d59d6rsnwf04595gdwdx4bhf[kp-secure.configs.hw.custom-configs.check-kp-test-prod-migrations.json]') | json }}
s3config-graphdata_secret: {{ salt.yav.get('sec-01dd89p2yfs4shdwcf5tj0dy3d[item]') | json }}
s3config-static_secret: {{ salt.yav.get('sec-01dk9e1bfh44sd8tedk6ses564[item]') | json }}
{% set mysql_ping_secret='sec-01dyjd0fer7cjdm4kdmjdhv41r' %}
{% else %}
s3config-static_secret: {{ salt.yav.get('sec-01drjyghfx3497pn765v1r7h3z[item]') | json }}
{% set mysql_ping_secret='sec-01dyjc1fx7evbrt5zv7er7t3s6' %}
{% endif %}

flyway:
  user: {{ salt.yav.get('sec-01d5bv54696y3envkd374gkcr6[user]') }}
  password: {{ salt.yav.get('sec-01d5bv54696y3envkd374gkcr6[password]') }}

{% if yaenv in ["production", "prestable"] %}
{% set tvmtool_sec_id = 'sec-01dfbgxmmz0n3pw6mpa6yr68gc' %}
{% elif yaenv in ["development"] %}
{% set tvmtool_sec_id = 'sec-01dfbgthf0f614c38dw9ebzdxn' %}
{% else %}
{% set tvmtool_sec_id = 'sec-01dfbh0h9jqatma82mkmkd9tf4' %}
{% endif %}
tvmtool_id: {{ salt.yav.get(tvmtool_sec_id+"[id]") }}
tvmtool_secret: {{ salt.yav.get(tvmtool_sec_id+"[secret]") | json }}

mysql_ping_config: {{ salt.yav.get(mysql_ping_secret)[yaenv] | json }}

{% if yaenv in ["production", "testing"] %}
mdb_oauth_token: {{ salt.yav.get('sec-01d2cny7z1jvg9syc695bbew2v[mdb_oauth_token]') | json }}
mdb_redis_cluster: {{ 'mdbg0j13s3mtg1r1ddrg' if is_stable else 'mdbvprcleht2k125ngl5' }}
{% endif %}

{% set yaenv = grains['yandex-environment'] %}
{% set _pre = "-pre" if grains['yandex-environment'] in [ 'stress', 'prestable', 'qa', 'testing' ] else "" %}

tls_tickets:
  keyname: kino-backoffice.{{"" if grains['yandex-environment'] in ['production', 'prestable'] else "tst."}}yandex-team.ru.key
  robot:  # default robot name is robot-media-salt
    srckeydir: kino_backoffice
    {% set salt_secret = 'sec-01d5p4y6bmew51682bzkszycph' if yaenv in ['production', 'prestable'] else 'sec-01d5pj63a69s7cyp01rbmwy0ff' %}
    ssh_key: {{salt.yav.get(salt_secret)['robot_media_salt_tls_tickets_id_rsa']| json}}
  monitoring:
    threshold_age: '30 hours ago'

nginx:
  lookup:
    listens:
      listen_ssl:
        ports: ['443 spdy', '[::]:443 spdy']
        ssl:
          ssl_stapling: 'on'
          ssl_stapling_verify: 'on'
          ssl_trusted_certificate: /etc/nginx/ssl/yandexca.pem
          resolver: '[2a02:6b8:0:3400::1] ipv6=only'
          ssl_stapling_responder: 'http://yandex.ocsp-responder.com/'
          ssl_ciphers: 'kEECDH+AESGCM+AES128:kEECDH+AES128:kRSA+AESGCM+AES128:!RC4:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2'
          ssl_dhparam: '/etc/nginx/ssl/dhparam.pem'
          ssl_prefer_server_ciphers: 'on'
          ssl_protocols:
            - TLSv1
            - TLSv1.1
            - TLSv1.2
          ssl_session_cache: "shared:{{grains['conductor']['group']}}:128m"
          ssl_session_timeout: '28h'
          add_header: 'Strict-Transport-Security "max-age=31536000; includeSubdomains; preload"'
    log_params:
      name: 'access-log'
      access_log_name: access.log
    confd_params:
      access_log: '/var/log/nginx/access_post.log postformat'
    confd_addons:
      "02-accesslog_post": |
        log_format postformat '[$time_local] $http_host $remote_addr "$request" '
                              '$status "$http_referer" "$http_user_agent" '
                              '"$http_cookie" $request_time $upstream_cache_status '
                              '$bytes_sent "$upstream_addr" "$upstream_status" '
                              '"$upstream_response_time" "$request_body"';

push_client:
  clean_push_client_configs: True
  port: 8088
  stats:
{% if yaenv in ['prestable', 'production'] %}
    - name: logbroker
      fqdn: logbroker{{ _pre|d("", true) }}.yandex.net
      port: default
      server_lite: False
      sszb: False
      proto: rt
      ident: kinopoisk-stat
      logs:
        - file: kino-vod-bo/kinopoisk-stat/onlines-availability.log
          log_type: onlines-availability-log
{% endif %}
  logs:
    - file: nginx/access.log
    - file: nginx/error.log
    - file: nginx/access_post.log
      send_delay: 1
    - file: kino-vod-bo/app/app.log
      send_delay: 1
    - file: kino-vod-bo/trace/trace.log
      send_delay: 1
    - file: kino-vod-bo/app/output/stdout
      send_delay: 1
    - file: kino-vod-bo/app/output/stderr
      send_delay: 1

{% set yav_sec='sec-01d5413k26mjnhjce4swmmp47c' if yaenv == 'production' else 'sec-01d4wky6mzb2xa23n128bzn0cm'%}

certificates:
  {% if yaenv == 'production' %}
  contents:
    kino-backoffice.yandex-team.ru.crt: {{ salt.yav.get(yav_sec)['crt']| json }}
    kino-backoffice.yandex-team.ru.key: {{ salt.yav.get(yav_sec)['key']| json }}
  path: "/etc/nginx/ssl"
  {% else %}
  contents:
    wc.tst.kinotv.yandex.net.pem: {{ salt.yav.get(yav_sec)['pem']| json }}
  path: "/etc/nginx/ssl"
  {% endif %}

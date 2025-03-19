secrets:
  aab_token: {{salt.yav.get('sec-01d31np62cn0rn63wkfhwz68f2')['aab_token_production']|json}}

{% set yaenv = grains['yandex-environment'] %}
{% set _pre = "-pre" if yaenv in [ 'stress', 'prestable', 'qa', 'testing' ] else "" %}
{% if yaenv in ['production', 'prestable'] %}
{% set tls_ticket_name = 'tv.yandex.ru.key' %}
{% else %}
{% set tls_ticket_name = 'tv.tst.yandex.ru.key' %}
{% endif %}

{% if yaenv == 'production' -%}
{% set cry, react, pda, sitemap = ('cryprox.yandex.net', 'new.tv.yandex.ru', 'new.pda.tv.yandex.ru', 'https://yastatic.net/s3/tv-frontend/sitemap/production') -%}
{% elif yaenv == 'prestable' -%}
{% set cry, react, pda, sitemap = ('cryprox.yandex.net', 'new.beta.tv.yandex.ru', 'new.pda.beta.tv.yandex.ru', 'http://tv-frontend.s3.mdst.yandex.net/sitemap/prestable') -%}
{% else -%}
{% set cry, react, pda, sitemap = ('cryprox-test.yandex.net', 'new.testing.tv.yandex.ru', 'new.pda.testing.tv.yandex.ru', 'http://tv-frontend.s3.mdst.yandex.net/sitemap/testing') -%}
{% endif -%}

tls_tickets:
  keyname: {{ tls_ticket_name }}
  robot:  # default robot name is robot-media-salt
    srckeydir: tv_front
  monitoring:
    threshold_age: '30 hours ago'

front:
  sitemap_path: {{sitemap}}

nginx:
  lookup:
    listens:
      listen_ssl:
        ports: ['443 spdy', '[::]:443 spdy']
        ssl:
{% if yaenv == 'development' %}
          ssl_certificate: '/etc/nginx/ssl/dev.tv.yandex.ru.pem'
          ssl_certificate_key: '/etc/nginx/ssl/dev.tv.yandex.ru.pem'
{% elif yaenv == 'testing' %}
          ssl_certificate: '/etc/nginx/ssl/testing.tv.yandex.ru.pem'
          ssl_certificate_key: '/etc/nginx/ssl/testing.tv.yandex.ru.pem'
{% elif yaenv == 'prestable' %}
          ssl_certificate: '/etc/nginx/ssl/beta.tv.yandex.ru.pem'
          ssl_certificate_key: '/etc/nginx/ssl/beta.tv.yandex.ru.pem'
{% else %}
          ssl_certificate: '/etc/nginx/ssl/tv.yandex.ru.pem'
          ssl_certificate_key: '/etc/nginx/ssl/tv.yandex.ru.pem'
{% endif %}
          ssl_trusted_certificate: /etc/nginx/ssl/yandexca.pem
          ssl_ciphers: 'kEECDH+AESGCM+AES128:kEECDH+AES128:kRSA+AESGCM+AES128:kRSA+AES128:DES-CBC3-SHA:!RC4:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2'
          ssl_dhparam: '/etc/nginx/ssl/dhparam.pem'
          ssl_prefer_server_ciphers: 'on'
          ssl_protocols:
            - TLSv1
            - TLSv1.1
            - TLSv1.2
          ssl_session_cache: "shared:{{grains['conductor']['group']}}:128m"
          ssl_session_timeout: '28h'
          ssl_session_ticket_key:
            - /etc/nginx/ssl/tls/{{tls_ticket_name}}
            - /etc/nginx/ssl/tls/{{tls_ticket_name}}.prev
            - /etc/nginx/ssl/tls/{{tls_ticket_name}}.prevprev
          resolver: '[::1] ipv6=only'
    log_params:
      name: 'access-log-content-front'
      access_log_name: access.log
      custom_fields: ["http_x_yandex_https=$http_x_yandex_https", "http_x_yandex_l7=$http_x_yandex_l7", "request_method=$request_method", "http_x_aab_proxy=$http_x_aab_proxy", "http_x_no_aab_proxy=$http_x_no_aab_proxy", "loop_host=$http_x_loop", "upstream_connect_time=$upstream_connect_time"]
    logrotate:
      frequency: hourly
      rotate: 168
      delaycompress: False
      compress: False

push_client:
  clean_push_client_configs: True
  port: 8088
  stats:
    - name: logbroker
      fqdn: logbroker{{ _pre|d("", true) }}.yandex.net
      port: default
      server_lite: False
      sszb: False
      proto: rt
      ident: content_front
      logs:
        - file: nginx/access.log
          log_type: access-log


{% if yaenv == 'development' %}
certificates:
  contents:
    dev.tv.yandex.ru.pem: {{ salt.yav.get('sec-01d5432w626r2q6y4fn0458sd8')['dev']| json }}
  path: "/etc/nginx/ssl"
{% endif %}

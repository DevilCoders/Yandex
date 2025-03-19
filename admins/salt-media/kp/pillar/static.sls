{% set is_stable = grains["yandex-environment"] in ["production", "prestable"] %}
{% set is_prestable = grains["yandex-environment"] in ["prestable"] %}
{% set fqdn = grains['fqdn'] %}
{% set rdc = grains['conductor']['root_datacenter'] %}

loggiver:
  lookup:
    spath: salt://static/files/loggiver.pattern

push_client:
  clean_push_client_configs: true
  ignore_pkgver: true
  port: 8088
  stats:
    - name: logbroker
      proto: pq
      port: 2135
      tvm_client_id_testing: 2015957
      tvm_client_id_production: 2015955
      tvm_server_id: 2001059
      tvm_secret_file: .tvm_secret
      tvm_secret: {{salt.yav.get('sec-01dn779402s1gmjkx27yz8axed[secret]' if is_stable else 'sec-01dn77aax4x7y36jay704ts2y1[secret]')|json}}
      fqdn: logbroker.yandex.net
      logger:
        mode: file
        file: /var/log/statbox/logbroker.log
        telemetry_interval: -1
        remote: 0
      logs:
        - file: nginx/access.log
          topic: /kinopoisk/logs/kp-static-log
  logs:
    - file: nginx/access.log
      fakename: {{ grains.conductor.group }}/nginx/access.log

nginx:
  lookup:
    enabled: True
    log_params:
      name: 'kinopoisk-tskv-static-log'
      access_log_name: access.log
      custom_fields:
        - "antirobot_status=$antirobot_status"
        - "bad_referer=$bad_referer"
        - "bad_referer_redirect=$bad_referer_redirect"
    confd_params:
      keepalive_timeout: '75 60'
      sendfile: 'off'
      output_buffers: '16 64k'
      gzip_buffers: '128 8k'
      gzip_comp_level: '7'
      gzip_proxied: 'any'
      gzip_vary: 'on'
      gzip_types: "application/json application/javascript application/vnd.ms-fontobject application/x-font-ttf application/x-javascript application/xml application/xml+rss font/opentype image/svg+xml image/x-icon text/css text/javascript text/plain text/xml"
      add_header: 'Cache-Control "public"'
      expires: '4h'
      request_id_from_header: 'on'
      'include':
        - 'include/upstreams.conf'
    logrotate:
      frequency: hourly
      rotate: 24
      delaycompress: False
      compress: False

{% if is_stable %}
rsyncd:
  secrets:
    master: {{salt.yav.get('sec-01ct3e5jp5dw7mjfdb9m8md0j5[master]')|json}}
    static: {{salt.yav.get('sec-01ct3e5jp5dw7mjfdb9m8md0j5[static]')|json}}
{% endif %}

basic_auth:
  google: {{ salt.yav.get('sec-01ecn2wdn5457s1eja2n88cnwg[google]')|json }}

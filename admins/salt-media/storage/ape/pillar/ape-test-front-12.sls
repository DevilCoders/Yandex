yasmagent:
  instance-getter:
      - echo {{ grains['conductor']['fqdn'] }} a_prj_none a_ctype_testing a_geo_{{ grains['conductor']['root_datacenter'] }} a_tier_none a_itype_apefront

push_client:
  clean_push_client_configs: True
  port: default
  check:
    status:
      status: True
  stats:
    - name: push-client-cocaine-runtime
      check:
        lag-size: 10485760
        commit-time: 600
        send-time: 600
      fqdn: logbroker.yandex.net
      port: default
      server_lite: False
      sszb: False
      proto: pq
      tvm_client_id_testing: 2019209
      tvm_client_id_production: 2019209
      tvm_server_id: 2001059
      tvm_secret: {{salt.yav.get('sec-01e3mhk96axhgcc9wbn6q8zw6p[client_secret]') | json}}
      tvm_secret_file: .tvm_secret
      logs:
        - file: cocaine-runtime/cocaine.log
          topic: cocaine-test-front/cocaine-log
          compress: gzip
          fakename: False
        - file: cocaine-runtime/core.log
          topic: cocaine-test-front/cocaine-log
          compress: gzip
          fakename: False
    - name: push-client-nginx
      check:
        lag-size: 10485760
        commit-time: 600
        send-time: 600
      fqdn: logbroker.yandex.net
      port: default
      server_lite: False
      sszb: False
      proto: pq
      tvm_client_id_testing: 2019209
      tvm_client_id_production: 2019209
      tvm_server_id: 2001059
      tvm_secret: {{salt.yav.get('sec-01e3mhk96axhgcc9wbn6q8zw6p[client_secret]') | json}}
      tvm_secret_file: .tvm_secret
      logs:
        - file: nginx/access_tskv.log
          topic: cocaine-test-front/cocaine-log
          compress: gzip
          fakename: False

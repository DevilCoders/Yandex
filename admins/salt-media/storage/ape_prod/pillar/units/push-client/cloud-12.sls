push_client:
  clean_push_client_configs: True
  port: default
  check:
    status:
      status: True
      logs: False
  stats:
    - name: cocaine-backend
      check:
        lag-size: 10485760
        commit-time: 900
        send-time: 900
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
          topic: cocaine-backend/cocaine-log
          compress: gzip
          fakename: False
        - file: cocaine-runtime/core.log
          topic: cocaine-backend/cocaine-log
          compress: gzip
          fakename: False

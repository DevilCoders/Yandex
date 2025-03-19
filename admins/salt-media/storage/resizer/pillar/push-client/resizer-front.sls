push_client:
  clean_push_client_configs: True
  port: default
  check:
    status:
      status: True
      logs: False
  stats:
    - name: resizer-front
      check:
        lag-size: 10485760
        commit-time: 600
        send-time: 600
      fqdn: logbroker.yandex.net
      port: default
      server_lite: False
      sszb: False
      proto: pq
      tvm_client_id_production: 2021057
      tvm_server_id: 2001059
      tvm_secret: {{salt.yav.get('sec-01ebkd0e068q1sw3wd365pd993[client_secret]') | json}}
      tvm_secret_file: .tvm_secret
      logs:
        - file: nginx/tskv.log
          topic: resizer-front/resizer-access-log
          fakename: False
          compress: zstd

  logs:
    - file: statbox/push-client-logbroker.log

push_client:
  clean_push_client_configs: True
  port: default
  check:
    status:
      status: True
  stats:
    - name: push-client-logbroker
      check:
        lag-size: 104857600
        commit-time: 600
        send-time: 600
      fqdn: logbroker.yandex.net
      port: default
      server_lite: False
      sszb: False
      proto: pq
      tvm_client_id_testing: 2002150
      tvm_client_id_production: 2002150
      tvm_server_id: 2001059
      tvm_secret: {{salt.yav.get('sec-01dq7m3y17dqpfxdj5sf0yw7pv[client_secret]') | json}}
      tvm_secret_file: .tvm_secret
      logs:
        - file: nginx/tskv.log
          topic: avatars-proxy/mds-access-log
          fakename: False
          sid: random
          compression: zstd

    - name: push-client-logbroker-int
      check:
        commit-time: 600
        send-time: 600
      fqdn: logbroker.yandex.net
      port: default
      server_lite: False
      sszb: False
      proto: pq
      tvm_client_id_testing: 2002150
      tvm_client_id_production: 2002150
      tvm_server_id: 2001059
      tvm_secret: {{salt.yav.get('sec-01dq7m3y17dqpfxdj5sf0yw7pv[client_secret]') | json}}
      tvm_secret_file: .tvm_secret
      logs:
        - file: nginx/int-tskv.log
          topic: avatars-proxy/mds-int-access-log
          fakename: False
          compress: zstd

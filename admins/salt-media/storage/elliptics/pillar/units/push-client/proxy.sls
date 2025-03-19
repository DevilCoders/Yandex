push_client:
  clean_push_client_configs: True
  port: default
  check:
    status:
      status: True
      logs: False
  stats:
    - name: proxy
      check:
        lag-size: 104857600
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
      drop_on_error: 1
      logs:
        - file: nginx/tskv.log
          fakename: False
          topic: elliptics-proxy/mds-access-log
          compress: zstd
        - file: nginx/s3-tskv.log
          compress: zstd
          topic: elliptics-proxy/s3-access-log
          fakename: False

    - name: proxy-int
      check:
        commit-time: 600
        send-time: 600
      fqdn: logbroker.yandex.net
      port: default
      server_litle: False
      sszb: False
      proto: pq
      tvm_client_id_testing: 2019209
      tvm_client_id_production: 2019209
      tvm_server_id: 2001059
      tvm_secret: {{salt.yav.get('sec-01e3mhk96axhgcc9wbn6q8zw6p[client_secret]') | json}}
      tvm_secret_file: .tvm_secret
      drop_on_error: 1
      logs:
        - file: nginx/int-tskv.log
          topic: elliptics-proxy/mds-int-access-log
          fakename: False
          sid: random
          compress: zstd

    - name: mds-proxy
      check:
        lag-size: 10485760
        commit-time: 600
        send-time: 600
      fqdn: logbroker.yandex.net
      port: default
      server_litle: False
      sszb: False
      proto: pq
      tvm_client_id_testing: 2019209
      tvm_client_id_production: 2019209
      tvm_server_id: 2001059
      tvm_secret: {{salt.yav.get('sec-01e3mhk96axhgcc9wbn6q8zw6p[client_secret]') | json}}
      tvm_secret_file: .tvm_secret
      drop_on_error: 1
      logs:
        - file: mds/tskv.log
          topic: elliptics-proxy/mds-proxy-log
          fakename: False
          compress: zstd

    - name: disk-downloader
      check:
        lag-size: 104857600
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
      drop_on_error: 1
      logs:
        - file: mds/access.log
          topic: disk-downloader/lenulca-access-log
          compress: zstd

  logs:
    - file: statbox/push-client-logbroker-proxy.log

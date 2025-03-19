push_client:
  clean_push_client_configs: True
  port: default
  check:
    status:
      status: True
  stats:
    - name: proxy
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
        - file: nginx/tskv.log
          compress: zstd
          topic: elliptics-test-proxies/mds-access-log
          fakename: False
        - file: nginx/s3-tskv.log
          compress: zstd
          topic: elliptics-test-proxies/s3-access-log
          fakename: False

    - name: proxy-int
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
      logs:
        - file: nginx/int-tskv.log
          topic: elliptics-test-proxies/mds-int-access-log
          compress: zstd
          fakename: False
          sid: random

    - name: s3-billing
      check:
        lag-size: 10485760
        commit-time: 600
        send-time: 600
      fqdn: lbkx.logbroker.yandex.net
      port: default
      server_litle: False
      sszb: False
      proto: pq
      tvm_client_id_testing: 2019209
      tvm_client_id_production: 2019209
      tvm_server_id: 2001059
      tvm_secret: {{salt.yav.get('sec-01e3mhk96axhgcc9wbn6q8zw6p[client_secret]') | json}}
      tvm_secret_file: .tvm_secret
      logs:
        - file: s3/billing/report.log
          topic: yc/yandex/billing-object-storage-test
          fakename: False
          sid: random

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
      logs:
        - file: mds/tskv.log
          topic: elliptics-test-proxies/mds-proxy-log
          compress: zstd
          fakename: False

  logs:
    - file: statbox/push-client-logbroker-proxy.log

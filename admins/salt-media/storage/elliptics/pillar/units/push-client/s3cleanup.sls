push_client:
  clean_push_client_configs: True
  port: default
  check:
    status:
      status: True
      logs: False
  stats:
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
          topic: yc/yandex/billing-object-storage
          fakename: False
          sid: random

    # Temporary block for handmade dataflow to billing, must be removed in future
    - name: mds-billing
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
        - file: mds-billing/billing-object-storage-test.log
          topic: yc/yandex/billing-object-storage-test
          fakename: False
          sid: random
        - file: mds-billing/billing-avatars-test.log
          topic: yc/yandex/billing-avatars-test
          fakename: False
          sid: random
        - file: mds-billing/billing-mds-test.log
          topic: yc/yandex/billing-mds-test
          fakename: False
          sid: random
        - file: mds-billing/billing-object-storage.log
          topic: yc/yandex/billing-object-storage
          fakename: False
          sid: random
        - file: mds-billing/billing-avatars.log
          topic: yc/yandex/billing-avatars
          fakename: False
          sid: random
        - file: mds-billing/billing-mds.log
          topic: yc/yandex/billing-mds
          fakename: False
          sid: random


  logs:
    - file: statbox/push-client-logbroker-s3cleanup.log

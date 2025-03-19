push_client:
  clean_push_client_configs: True
  port: default
  check:
    status:
      status: True
  stats:
    - name: disk
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
        - file: nginx/downloader/access.log
          topic: disk-downloader/ydisk-downloader-access-log
          compress: gzip
          fakename: False
    - name: mds_cleanup
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
        - file: mds_cleanup/tskv.log
          topic: elliptics-test-proxies/mds-proxy-log
          pipe: "/bin/sed -u 's/mds_cleanup-log/mds-proxy-log/g;s/expiration_date/expire_at/g'"
          compress: zstd
          fakename: False

    - name: rt
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
          topic: elliptics-test-storage/mds-access-log
          compress: zstd
          fakename: False
    - name: cocaine-test-backend
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
        - file: cocaine-core/cocaine-tskv.log
          topic: cocaine-test-backend/cocaine-log
          compress: gzip
          fakename: False
        - file: cocaine-core/cocaine-logging-tskv.log
          topic: cocaine-test-backend/cocaine-log
          compress: gzip
          fakename: False

    # https://st.yandex-team.ru/MDS-13435
    - name: elliptics-small
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
        - file: elliptics/tskv-small.log
          topic: elliptics-test-storage/mds-elliptics-small-record-log
          compress: zstd
          fakename: False

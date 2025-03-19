billing:
  passport:
    e2e_passport_uid: 867733839
  notifications:
    enabled: True
    server_url: https://console.cloud.yandex.net
  logbroker:
    client_id: yc-billing-pollster
    export:
      enabled: True
      producer: yc
  solomon:
    cluster: prod
  billing_account_cardinality: 1000
  id_prefix: dn2
  s3:
    private_api: https://storage-idm.private-api.cloud.yandex.net:1443
    url: https://storage.yandexcloud.net
    endpoint_url: https://storage.yandexcloud.net
    reports_bucket: reports
  api:
    auth_enabled: True
    tvm_enabled: True
    tvm_allowed_services:
      - "2000599"
      - "2000601"
  balance_api:
    server_url: https://balance.yandex.ru
    tvm_destination: 2001902
  balance:
    url: https://balance-xmlrpc-tvm.paysys.yandex.net:8004/xmlrpctvm
    use_tvm: True
    manager_uid: 437359512
    balance_tvm_client_id: "2000599"
    timeout: 300
  tvm:
    url: https://tvm-api.yandex.net
    client_id: "2000497"
  uploader:
    enabled: True
    parallel: True
    polling_delay: 1
    source:
      logbroker:
        auth:
          client_id: "2000497"
        send_sensors: True
        send_batch: 100
        topics:
          - name: yc-mdb--billing-mdb-instance
          - name: yc-mdb--billing-mdb-backup
          - name: yc--billing-compute-instance
          - name: yc--billing-compute-snapshot
          - name: yc--billing-compute-image
          - name: yc--billing-object-storage
          - name: yc--billing-object-requests
            sink: ydb.s3
            limit: 10
            max_messages_count: 10
          - name: yc--billing-nbs-volume
            limit: 1000
            max_messages_count: 1000
          - name: yc--billing-sdn-traffic
          - name: yc--billing-sdn-fip
          - name: yc--billing-ai-requests
          - name: yc--billing-nlb-balancer
          - name: yc--billing-nlb-traffic
          - name: yc--billing-sqs-requests
          - name: yc--billing-ms-sql-report
  engine:
    logbroker:
      topics:
        - rt3.vla--yc-mdb--billing-mdb-instance
        - rt3.vla--yc-mdb--billing-mdb-backup
        - rt3.vla--yc--billing-compute-instance
        - rt3.vla--yc--billing-compute-snapshot
        - rt3.vla--yc--billing-compute-image
        - rt3.vla--yc--billing-object-storage
        - rt3.vla--yc--billing-object-requests
        - rt3.vla--yc--billing-nbs-volume
        - rt3.vla--yc--billing-sdn-traffic
        - rt3.vla--yc--billing-sdn-fip
        - rt3.vla--yc--billing-ai-requests
        - rt3.vla--yc--billing-nlb-balancer
        - rt3.vla--yc--billing-nlb-traffic
        - rt3.vla--yc--billing-sqs-requests
        - rt3.vla--yc--billing-ms-sql-report

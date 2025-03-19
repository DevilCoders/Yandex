billing:
  passport:
    e2e_passport_uid: 874988587
  solomon:
    cluster: preprod
  billing_account_cardinality: 1000
  logbroker:
    export:
      enabled: True
      producer: yc-pre
  id_prefix: a6q
  s3:
    private_api: https://storage-idm.private-api.cloud-preprod.yandex.net:1443
    url: https://storage.cloud-preprod.yandex.net
    endpoint_url: https://storage.cloud-preprod.yandex.net
    reports_bucket: reports
  api:
    auth_enabled: True
    tvm_enabled: True
    tvm_allowed_services:
      - "2000599"
      - "2000601"
  notifications:
    enabled: True
    server_url: https://console-preprod.cloud.yandex.net
  balance:
    url: https://balance-xmlrpc-tvm-test.paysys.yandex.net:8004/xmlrpctvm
    use_tvm: True
    balance_tvm_client_id: "2000601"
  tvm:
    url: https://tvm-api.yandex.net
    client_id: "2000508"
  uploader:
    enabled: True
    parallel: True
    polling_delay: 1
    source:
      logbroker:
        send_sensors: True
        send_batch: 100
        topics:
          - name: yc-mdb-pre--billing-mdb-instance
          - name: yc-mdb-pre--billing-mdb-backup
          - name: yc-pre--billing-compute-instance
          - name: yc-pre--billing-compute-snapshot
          - name: yc-pre--billing-compute-image
          - name: yc-pre--billing-object-storage
          - name: yc-pre--billing-object-requests
            sink: ydb.s3
            limit: 10
            max_messages_count: 10
          - name: yc-pre--billing-nbs-volume
          - name: yc-pre--billing-sdn-traffic
          - name: yc-pre--billing-sdn-fip
          - name: yc-pre--billing-ai-requests
          - name: yc@preprod--billing-nlb-balancer
          - name: yc@preprod--billing-nlb-traffic
          - name: yc@preprod--billing-sqs-requests
          - name: yc@preprod--billing-ms-sql-report
  engine:
    logbroker:
      topics:
        - rt3.vla--yc-mdb-pre--billing-mdb-instance
        - rt3.vla--yc-mdb-pre--billing-mdb-backup
        - rt3.vla--yc-pre--billing-compute-instance
        - rt3.vla--yc-pre--billing-compute-snapshot
        - rt3.vla--yc-pre--billing-compute-image
        - rt3.vla--yc-pre--billing-object-storage
        - rt3.vla--yc-pre--billing-object-requests
        - rt3.vla--yc-pre--billing-nbs-volume
        - rt3.vla--yc-pre--billing-sdn-traffic
        - rt3.vla--yc-pre--billing-sdn-fip
        - rt3.vla--yc-pre--billing-ai-requests
        - rt3.vla--yc@preprod--billing-nlb-balancer
        - rt3.vla--yc@preprod--billing-nlb-traffic
        - rt3.vla--yc@preprod--billing-sqs-requests
        - rt3.vla--yc@preprod--billing-ms-sql-report

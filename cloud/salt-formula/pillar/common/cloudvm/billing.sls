billing:
  id_prefix: ac0
  billing_account_cardinality: 1

  api:
    auth_enabled: False
    tvm_enabled: False

  engine:
    default_partitions: 1
    logbroker:
      topics: []
    queue:
      backend: kikimr
      name: engine
      default_partitions: 1
      worker: []
  s3:
    private_api: https://storage-idm.private-api.cloud-preprod.yandex.net:1443
    url: https://storage.cloud-preprod.yandex.net
    endpoint_url: https://storage.cloud-preprod.yandex.net
    reports_bucket: reports-cloudvm
  uploader:
    enabled: True
    parallel: True
    source:
      logbroker:
        auth:
          enabled: True
          client_id: "2000508"
        topics: []
  queue:
    backend: kikimr
    name: test
    default_partitions: 1
    worker: []
  solomon_queue:
    backend: kikimr
    name: solomon
    default_partitions: 1
    worker: []
  nginx:
    cert: "/etc/nginx/ssl/localhost.crt"
    key: "/etc/nginx/ssl/localhost.key"

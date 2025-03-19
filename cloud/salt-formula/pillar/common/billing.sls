billing:
  passport:
    e2e_passport_uid: 874988587
  queue:
    backend: kikimr
    name: billing
    default_partitions: 15
    worker:
      - lock_timeout: 3600
        queued_at_timeout: 1
        polling_delay: 60
        limit: 10
        queues:
          - backups
          - exporter
        fetch_periodic: 1
      - lock_timeout: 7200
        queued_at_timeout: 1
        polling_delay: 60
        limit: 10
        queues:
          - reports.exporter.s3.once
          - reports.exporter.s3.daily
        fetch_periodic: 1
      - lock_timeout: 600
        instances: 2
        queued_at_timeout: 1
        polling_delay: 5
        graceful_shutdown: true
        limit: 10
        queues:
          - paysystem
          - balance
          - identity
        fetch_periodic: 1
  nginx:
    cert: "/etc/ssl/private/billing.private.api.pem"
    key: "/etc/ssl/private/billing.private.api.pem"
  solomon:
    base_url: http://solomon.yandex.net/push
    project: yc_billing
    period: 60
    cluster: test
  solomon_queue:
    backend: kikimr
    name: solomon
    default_partitions: 15
    worker:
      - lock_timeout: 600
        queued_at_timeout: 1
        polling_delay: 30
        limit: 5
        queues:
          - meta
          - cashier
          - scheduler
          - presenter
          - uploader
        fetch_periodic: 1
  monetary_grants:
    default_grants_enabled: True
    grants_by_passport_enabled: False
    idempotency:
      - 'request.real_card_id'
      - 'client.balance_client_id'
      - 'request.passport_uid'
      - 'request.passport_phone'
      - 'request.passport_email'
      - 'person.company.inn'
      - 'request.connect_domain_id'
    bad_karma:
      - 100
  billing_account_cardinality: 1
  monitoring_endpoint: http://[::1]:6464/health?monrun=1
  monitoring_private_endpoint: http://[::1]:6465/health?monrun=1
  balance_api:
    server_url: https://user-balance.greed-ts.paysys.yandex.ru
    tvm_destination: 2001900
  balance:
    url: http://greed-ts.paysys.yandex.ru:8002/xmlrpc
    service_id: 143
    firm_id: 123
    payment_product_id: 509071
    manager_uid: 98700241
    default_operator_uid: 45370199
    service_token: cloud_3752c735922f2705f15c947866592e7e
    use_tvm: False

  logbroker:
    host: vla.logbroker.yandex.net
    client_id: yc-billing-pollster-test
    port: 2135
    export:
      enabled: False
      destination: "2001059"
  uploader:
    enabled: False
    source:
      logbroker:
        polling_delay: 30
        auth:
          enabled: True
          client_id: "2000508"
          destination: "2001059"
        dc_list:
          - vla
        topics: []

  engine:
    logbroker:
      topics: []
      source_type: logbroker-grpc
    queue:
      backend: kikimr
      name: engine
      default_partitions: 30
      queue_partitions:
        min_processed_to: 5
        tariffication_scheduler: 5
        tarifficator: 30
        cashier.schedule: 5
        cashier.cash: 30
        presenter.schedule: 5
        presenter.present: 30
      worker:
        - queues: [cashier.schedule]
          instances: 1
          lock_timeout: 600
        - queues: [cashier.cash]
          instances: 4
          lock_timeout: 3600
        - queues: [presenter.schedule]
          instances: 1
          lock_timeout: 600
        - queues: [presenter.present]
          instances: 4
          lock_timeout: 3600
        - queues: [min_processed_to]
          instances: 1
          lock_timeout: 600

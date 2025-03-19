{% set api_image="registry.yandex.net/cloud/support-api" %}
{% set queue_image="registry.yandex.net/cloud/support-queue" %}
{% set api_tag="master-29.03.2022-f97f4b8" %}
{% set queue_tag=api_tag %}

support-api:
  hostname: "support.private-api.cloud-preprod.yandex.net"
  docker: "{{ api_image }}:{{ api_tag }}"
  ydb_client_version: 2
  kikimr:
    host: "grpcs://mkt-dn.ydb.cloud-preprod.yandex.net:2136"
    database: /pre-prod_global/mkt
    root: /pre-prod_global/mkt/support
    ydb_token_from_metadata: default-url
    root_ssl_cert_file: /etc/ssl/certs/ca-certificates.crt
    enable_logging: false
  st:
    url: 'https://st-api.yandex-team.ru'
    queue: 'CLOUDSUPAPI'
    doc_queue: 'CLOUDSUPAPI'
    call_queue: 'CLOUDSUPAPI'
    escalated: 'CLOUDSUPAPI'
    marketplace_queue: 'CLOUDSUPAPITWO'
    service_quotas_queue: 'CLOUDSUPAPI'
    valid_queues:
      - CLOUDSUPAPI
      - CLOUDSUPAPITWO
  cloud_tracker:
    url: 'https://api-integration-qa.directory.ws.yandex.net'
  base_url: 'https://support.private-api.cloud-preprod.yandex.net'
  notify_url: 'https://console-preprod.cloud.yandex.net'
  quotamanager_grpc: 'qm.private-api.cloud-preprod.yandex.net:4224'
  e2e:
    folder_id: aoe7ornk6096spa90bov
    cloud_id: aoevej5914i5dc695lbt
    billing_account_id: a6q2lbcnlhabnm1uje53
    enabled: true
  logbroker_export:
      host: logbroker.yandex.net
      port: 2135
      producer: yc-pre
      auth:
          enabled: True
          destination: 2001059
  features:
    quotamanager: true

support-queue:
  docker: "{{ queue_image }}:{{ queue_tag }}"
  sqs:
    queue_url: https://message-queue.api.cloud-preprod.yandex.net/aoeaql9r10cd9cfue7v6/ab40000000005s7d034r/support-api.fifo

support-solomon:
  cluster: preprod
  auth:
    enabled: true
    parameters:
      type: tvm
      destination: 2010242



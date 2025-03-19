{% set api_image="registry.yandex.net/cloud/support-api" %}
{% set queue_image="registry.yandex.net/cloud/support-queue" %}
{% set api_tag="master-29.03.2022-f97f4b8" %}
{% set queue_tag=api_tag %}

support-api:
  hostname: "support.private-api.cloud.yandex.net"
  docker: "{{ api_image }}:{{ api_tag }}"
  ydb_client_version: 2
  kikimr:
    host: "grpcs://mkt-dn.ydb.cloud.yandex.net:2136"
    database: /global/mkt
    root: /global/mkt/support
    ydb_token_from_metadata: default-url
    root_ssl_cert_file: /etc/ssl/certs/ca-certificates.crt
    enable_logging: false
  st:
    url: 'https://st-api.yandex-team.ru'
    queue: 'CLOUDSUPPORT'
    doc_queue: 'CLOUDDOCS'
    call_queue: 'CLOUDCONTACT'
    escalated: 'CLOUDPS'
    marketplace_queue: 'CLOUDMPPUBLISH'
    service_quotas_queue: 'CLOUDSUPPORT'
    valid_queues:
      - CLOUDSUPPORT
      - CLOUDLINETWO
      - CLOUDMPPUBLISH
  cloud_tracker:
    url: 'https://api-internal.directory.ws.yandex.net'
  base_url: 'https://support.private-api.cloud.yandex.net'
  notify_url: 'https://console.cloud.yandex.net'
  quotamanager_grpc: 'qm.private-api.cloud.yandex.net:4224'
  e2e:
    folder_id: ""
    cloud_id: ""
    billing_account_id: ""
    enabled: false
  logbroker_export:
      host: logbroker.yandex.net
      port: 2135
      producer: yc
      auth:
          enabled: True
          destination: 2001059
  features:
    quotamanager: true

support-queue:
  docker: "{{ queue_image }}:{{ queue_tag }}"
  sqs:
    queue_url: https://message-queue.api.cloud.yandex.net/b1gjqgj3hhvjen5iqakp/dj6000000000f81v02n0/support-api.fifo

support-solomon:
  cluster: prod
  auth:
    enabled: true
    parameters:
      type: tvm
      destination: 2010242

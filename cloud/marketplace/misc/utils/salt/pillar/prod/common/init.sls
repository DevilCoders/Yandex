{% set image="cr.yandex/crpo2khtm9ocefi307m8/marketplace-v1" %}
{% set tag="trunk-7307007" %}

common:
  docker_api: "{{ image }}:{{ tag }}"
  docker_queue: "{{ image }}-worker:{{ tag }}"
  docker_cli: "{{ image }}-cli:{{ tag }}"
  docker_user: "robot-marketplace"
  version: "{{ tag }}"
  loglevel: INFO
  yc_cli_version: "0.1-207.180919"
  yc_factory_version: "2021.07.15-18fcc01"
  hostname: "mkt.private-api.cloud.yandex.net"
  zone: ru-central1.internal
  log-reader:
    topic: yc@marketplace--logs
  api-hosts:
    - mkt-api-1a
    - mkt-api-2a
    - mkt-api-3a
    - mkt-api-1b
    - mkt-api-2b
    - mkt-api-3b
    - mkt-api-1c
    - mkt-api-2c
    - mkt-api-3c
  sec_packages:
    osquery: "4.8.0.1"
    osquery_cfg: "1.1.1.59"
  juggler:
    endpoint: juggler-api.search.yandex.net
    targets: AUTO

{% include 'common/api.sls' %}
{% include 'common/factory.sls' %}
{% include 'common/support-api.sls' %}


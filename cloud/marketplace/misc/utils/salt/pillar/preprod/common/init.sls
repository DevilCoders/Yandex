{% set image="cr.yandex/crpo2khtm9ocefi307m8/marketplace-v1" %}
{% set tag="trunk-7307007" %}

common:
  docker_api: "{{ image }}:{{ tag }}"
  docker_queue: "{{ image }}-worker:{{ tag }}"
  version: "{{ tag }}"
  loglevel: INFO
  yc_cli_version: "2018.12.05-ce8cec1"
  yc_factory_version: "2021.07.15-18fcc01"
  subscriptions_version: "master-04.04.2019-81ef1ba"
  docker_user: "robot-marketplace"
  hostname: "mkt.private-api.cloud-preprod.yandex.net"
  zone: ru-central1.internal
  log-reader:
    topic: yc-pre@marketplace--logs
  api-hosts:
    - mkt-api-1
    - mkt-api-2
    - mkt-api-3
  sec_packages:
    osquery: "4.8.0.1"
    osquery_cfg: "1.1.1.59"
  juggler:
    endpoint: juggler-api.search.yandex.net
    targets: AUTO

{% include 'common/api.sls' %}
{% include 'common/factory.sls' %}
{% include 'common/support-api.sls' %}

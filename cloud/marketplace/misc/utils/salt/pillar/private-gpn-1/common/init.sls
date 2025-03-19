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
  hostname: "mkt.private-api.gpn.yandexcloud.net"
  zone: ru-gpn-spb99
  log-reader:
    topic: yc-gpn@marketplace--logs
  api-hosts:
    - mkt-gpn-api-1
    - mkt-gpn-api-2
    - mkt-gpn-api-3
  sec_packages:
    osquery: "4.8.0.1"
    osquery_cfg: "1.1.1.59"
  juggler:
    endpoint: juggler-api.proxy.gpn.yandexcloud.net
    targets: "myt.juggler.proxy.gpn.yandexcloud.net:8991,vla.juggler.proxy.gpn.yandexcloud.net:8992,sas.juggler.proxy.gpn.yandexcloud.net:8993"

{% include 'common/api.sls' %}
{% include 'common/factory.sls' %}

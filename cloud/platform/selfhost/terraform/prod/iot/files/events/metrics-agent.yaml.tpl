add_host_label: yes
log:
  level: INFO
puller:
  metrics_buffer: 1
  scrape_interval: 15s
  scrape_timeout: 5s
pushers:
- type: SOLOMON
  batch_size: 9000
  push_timeout: "120s"
  url: "http://solomon.cloud.yandex-team.ru/push"
  project: "cloud_iot"
  cluster: "${solomon_shard_cluster}"
  service: "events-broker-ma"
  sources:
  - url: http://localhost:12580
    labels:
      application: events
- type: SOLOMON
  batch_size: 9000
  push_timeout: "120s"
  url: "http://solomon.cloud.yandex-team.ru/push"
  project: "cloud_iot"
  cluster: "${solomon_shard_cluster}"
  service: "events-broker-ma"
  sources:
  - url: http://localhost:9100/metrics
    labels:
      application: system

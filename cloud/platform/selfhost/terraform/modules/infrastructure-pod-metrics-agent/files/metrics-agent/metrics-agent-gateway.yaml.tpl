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
  project: "platform"
  cluster: "${solomon_shard_cluster}"
  service: "api_gateway_ma"
  sources:
  - url: http://localhost:9998/metrics
- type: SOLOMON
  batch_size: 9000
  push_timeout: "120s"
  url: "http://solomon.cloud.yandex-team.ru/push"
  project: "platform"
  cluster: "${solomon_shard_cluster}"
  service: "api_envoy_ma"
  sources:
  - url: http://localhost:9102/metrics
- type: SOLOMON
  batch_size: 9000
  push_timeout: "120s"
  url: "http://solomon.cloud.yandex-team.ru/push"
  project: "platform"
  cluster: "${solomon_shard_cluster}"
  service: "api-als_ma"
  sources:
  - url: http://localhost:4437/metrics
- type: SOLOMON
  batch_size: 9000
  push_timeout: "120s"
  url: "http://solomon.cloud.yandex-team.ru/push"
  project: "platform"
  cluster: "${solomon_shard_cluster}"
  service: "kubelet_ma"
  sources:
  - url: http://localhost:10255/metrics
- type: SOLOMON
  batch_size: 9000
  push_timeout: "120s"
  url: "http://solomon.cloud.yandex-team.ru/push"
  project: "platform"
  cluster: "${solomon_shard_cluster}"
  service: "node-exporter_ma"
  sources:
  - url: http://localhost:9100/metrics
- type: SOLOMON
  batch_size: 9000
  push_timeout: "120s"
  url: "http://solomon.cloud.yandex-team.ru/push"
  project: "platform"
  cluster: "${solomon_shard_cluster}"
  service: "fluentd_ma"
  sources:
  - url: http://localhost:24231/metrics
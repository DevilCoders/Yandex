/etc/kubernetes/manifests/metricsagent.yaml:
  file.managed:
    - source: salt://configs/manifests/metricsagent.yaml
    - makedirs: True

/etc/metricsagent/metricsagent.yaml:
  file.managed:
    - source: salt://configs/metricsagent/metricsagent.yaml
    - makedirs: True

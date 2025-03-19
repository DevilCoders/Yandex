collectable-group:
  group.present:
    - name: yc-metrics-collector
    - members:
      - root
      - yc-metrics-collector
    - require:
      - yc-metrics-collector-install

yc-metrics-collector-install:
  pkg.installed:
    - name: yc-metrics-collector
    - version: '0.1-7733.200723'

yc-metrics-collector:
  service.running:
    - names:
      - yc-metrics-collector
    - enable: True
    - watch:
      - file: /etc/yc/multiprocess_metric_collector/config.yaml
      - pkg: yc-metrics-collector-install
    - require:
      - yc-metrics-collector-install
      - collectable-group

/etc/yc/multiprocess_metric_collector/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/metrics-collector.yaml
    - makedirs: True
    - user: yc-metrics-collector
    - template: jinja
    - require:
      - yc-metrics-collector-install
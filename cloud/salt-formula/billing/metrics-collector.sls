{%- from "billing/map.jinja" import billing, versions with context %}

collectable-group:
  group.present:
    - name: yc-metrics-collector
    - members:
      - yc-billing
      - yc-billing-uploader
      - yc-billing-engine
      - yc-metrics-collector
    - require:
      - yc-billing-install
      - yc-billing-uploader-install
      - yc-billing-engine-install
      - yc-metrics-collector-install

yc-metrics-collector-install:
  yc_pkg.installed:
    - pkgs:
      - yc-metrics-collector

yc-metrics-collector:
  service.running:
    - names:
      - yc-metrics-collector
    - enable: True
    - watch:
      - file: /etc/yc/multiprocess_metric_collector/config.yaml
      - yc_pkg: yc-metrics-collector-install
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

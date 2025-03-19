{%- from "billing/map.jinja" import billing, versions with context %}

include:
  - billing

yc-billing-engine-install:
  yc_pkg.installed:
    - pkgs:
      - yc-billing-engine

yc-billing-engine:
  service.running:
    - names:
      - yc-billing-engine
    - require:
      - file: /etc/yc/billing-engine/env
      - file: /tmp/yc_billing_engine/prom_data
      - yc-billing-engine-install
    - enable: True
    - watch:
      - file: /etc/yc/billing-engine/engine.yaml
      - yc_pkg: yc-billing-engine-install

/etc/yc/billing-engine/engine.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/engine.yaml
    - makedirs: True
    - user: yc-billing-engine
    - template: jinja
    - require:
      - yc-billing-engine-install

/etc/yc/billing-engine/env:
  file.managed:
    - source: salt://{{ slspath }}/files/engine_env
    - makedirs: True
    - user: yc-billing-engine
    - template: jinja
    - require:
      - yc-billing-engine-install

/etc/tmpfiles.d/engine.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/engine_tempfiles.conf
    - template: jinja

/tmp/yc_billing_engine/prom_data:
  file.directory:
    - user: yc-billing-engine
    - group: yc-metrics-collector
    - file_mode: 644
    - dir_mode: 775
    - makedirs: True
    - recurse:
      - user
      - group
      - mode
    - require:
      - yc-billing-engine-install
      - collectable-group

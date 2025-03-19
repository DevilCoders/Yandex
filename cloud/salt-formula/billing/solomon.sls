{%- from "billing/map.jinja" import billing, versions with context %}

yc-billing-solomon:
  yc_pkg.installed:
    - name: yc-billing-solomon
  service.running:
    - names:
      - yc-billing-solomon
    - enable: True
    - watch:
      - file: /etc/yc/billing-solomon/solomon.yaml
      - yc_pkg: yc-billing-solomon

/etc/yc/billing-solomon/solomon.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/solomon.yaml
    - makedirs: True
    - user: yc-billing-solomon
    - template: jinja
    - require:
      - yc_pkg: yc-billing-solomon

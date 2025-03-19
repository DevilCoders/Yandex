{%- from "billing/map.jinja" import billing, versions with context %}

{% if billing.get('uploader', {}).get('enabled', False) %}

yc-billing-uploader-install:
  yc_pkg.installed:
    - pkgs:
      - yc-billing-uploader

yc-billing-uploader:
  service.running:
    - names:
      - yc-billing-uploader
    - enable: True
    - require:
      - file: /etc/yc/billing-uploader/env
      - file: /tmp/yc_billing_uploader/prom_data
      - yc-billing-uploader-install
    - watch:
      - file: /etc/yc/billing-uploader/uploader.yaml
      - file: /var/lib/yc/billing-uploader/secrets.yaml
      - yc_pkg: yc-billing-uploader-install

/etc/yc/billing-uploader/uploader.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/uploader.yaml
    - makedirs: True
    - user: yc-billing-uploader
    - template: jinja
    - require:
      - yc-billing-uploader-install

/etc/tmpfiles.d/uploader.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/uploader_tempfiles.conf
    - template: jinja

/var/lib/yc/billing-uploader/secrets.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/secrets/billing.yaml
    - user: yc-billing-uploader
    - mode: '0400'
    - makedirs: True
    - replace: False
    - require:
      - yc-billing-uploader-install

/etc/yc/billing-uploader/env:
  file.managed:
    - source: salt://{{ slspath }}/files/uploader_env
    - makedirs: True
    - user: yc-billing-uploader
    - template: jinja
    - require:
      - yc-billing-uploader-install

/tmp/yc_billing_uploader/prom_data:
  file.directory:
    - user: yc-billing-uploader
    - group: yc-metrics-collector
    - file_mode: 644
    - dir_mode: 775
    - makedirs: True
    - recurse:
      - user
      - group
      - mode
    - require:
      - yc-billing-uploader-install
      - collectable-group

{% endif %}

{%- from "billing/map.jinja" import versions with context %}

{%- import_yaml slspath+"/mon/billing.yaml" as monitoring %}
{%- include "common/deploy_mon_scripts.sls" %}

/etc/ssl/private/billing.private.api.pem:
  file.managed:
    - source: salt://nginx/nginx-certs/localhost.pem
    - makedirs: True
    - replace: False

include:
  - nginx
{%- set nginx_configs = ['billing-private-api.conf'] %}
{%- include 'nginx/install_configs.sls' %}

yc-billing-install:
  yc_pkg.installed:
    - pkgs:
      - yc-billing

yc-billing:
  service.running:
    - enable: True
    - names:
      - yc-billing
      - yc-billing-worker
      - yc-billing-private
    - require:
      - file: billing-db-migrated
      - file: /etc/yc/billing/env
      - file: /etc/yc/billing-private/env
      - file: /etc/yc/billing-worker/env
      - file: /tmp/yc_billing/prom_data
      - file: /tmp/yc_billing_private/prom_data
      - file: /tmp/yc_billing_worker/prom_data
      - yc-billing-install
    - watch:
      - file: /etc/yc/billing/config.yaml
      - file: /etc/yc/billing/wsgi.ini
      - file: /etc/yc/billing/wsgi-private.ini
      - file: /var/lib/yc/billing/system_account.json
      - file: /var/lib/yc/billing/secrets.yaml
      - yc_pkg: yc-billing-install

/etc/yc/billing/env:
  file.managed:
    - source: salt://{{ slspath }}/files/billing_env
    - makedirs: True
    - user: yc-billing
    - template: jinja
    - require:
      - yc-billing-install

/etc/yc/billing-private/env:
  file.managed:
    - source: salt://{{ slspath }}/files/billing_private_env
    - makedirs: True
    - user: yc-billing
    - template: jinja
    - require:
      - yc-billing-install

/etc/yc/billing-worker/env:
  file.managed:
    - source: salt://{{ slspath }}/files/billing_worker_env
    - makedirs: True
    - user: yc-billing
    - template: jinja
    - require:
      - yc-billing-install

/etc/tmpfiles.d/billing.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/billing_tempfiles.conf
    - template: jinja

/tmp/yc_billing/prom_data:
  file.directory:
    - user: yc-billing
    - group: yc-metrics-collector
    - file_mode: 644
    - dir_mode: 775
    - makedirs: True
    - recurse:
      - user
      - group
      - mode
    - require:
      - yc-billing-install
      - collectable-group

/tmp/yc_billing_private/prom_data:
  file.directory:
    - user: yc-billing
    - group: yc-metrics-collector
    - file_mode: 644
    - dir_mode: 775
    - makedirs: True
    - recurse:
      - user
      - group
      - mode
    - require:
      - yc-billing-install
      - collectable-group

/tmp/yc_billing_worker/prom_data:
  file.directory:
    - user: yc-billing
    - group: yc-metrics-collector
    - file_mode: 644
    - dir_mode: 775
    - makedirs: True
    - recurse:
      - user
      - group
      - mode
    - require:
      - yc-billing-install
      - collectable-group

/var/lib/yc/billing/secrets.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/secrets/billing.yaml
    - user: yc-billing
    - mode: '0400'
    - makedirs: True
    - replace: False
    - require:
      - yc-billing-install

/etc/yc/billing/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - user: yc-billing
    - group: yc-billing
    - require:
      - yc-billing-install

/etc/yc/billing/wsgi.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/wsgi.ini
    - template: jinja
    - user: yc-billing
    - group: yc-billing
    - require:
      - yc-billing-install

/etc/yc/billing/wsgi-private.ini:
  file.managed:
    - source: salt://{{ slspath }}/files/wsgi-private.ini
    - template: jinja
    - user: yc-billing
    - group: yc-billing
    - require:
      - yc-billing-install

/var/lib/yc/billing/system_account.json:
  file.managed:
    - source: salt://{{ slspath }}/files/system_account.json
    - user: yc-billing
    - group: yc-billing
    - mode: '0400'
    - replace: False
    - require:
      - yc-billing-install

billing-db-migrate:
  cmd.run:
    - names:
      - 'LC_ALL=C.UTF-8 LANG=C.UTF-8 /usr/bin/yc-billing-admin migrate'
    - cwd: /var/lib/yc/billing
    - require:
      - yc-billing-install
      - file: /etc/yc/billing/config.yaml
      - file: /var/lib/yc/billing/secrets.yaml
    - unless: test -e /etc/yc/billing/migrations/{{versions.get('yc-billing', 1)}}.migrated

billing-db-migrated:
  file.touch:
    - name: /etc/yc/billing/migrations/{{versions.get('yc-billing', 1)}}.migrated
    - makedirs: True
    - require:
      - cmd: billing-db-migrate

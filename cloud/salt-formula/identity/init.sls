{%- set lb_endpoints = grains['cluster_map']['load_balancer']['endpoints'] -%}
include:
  - nginx
  - scms
  - osquery

yc-identity:
  yc_pkg.installed:
    - pkgs:
      - yc-identity
      - ssl-cert

  service.running:
    - enable: True
    - names:
      - yc-identity-public
      - yc-identity-private
    - require:
      - yc_pkg: yc-identity
      - file: /etc/yc/identity/config.toml
      - file: /etc/yc/identity/wsgi.ini
      - file: /var/lib/yc/identity/system_account_keys.json
    - watch:
      - file: /etc/yc/identity/config.toml
      - file: /etc/yc/identity/wsgi.ini
      - yc_pkg: yc-identity

/etc/yc/identity/config.toml:
  file.managed:
    - source: salt://{{ slspath }}/config.toml
    - template: jinja
    - require:
      - yc_pkg: yc-identity
      - file: /var/lib/yc/identity/tvm_client_secret

/etc/yc/identity/wsgi.ini:
  file.managed:
    - source: salt://{{ slspath }}/wsgi.ini
    - template: jinja
    - require:
      - yc_pkg: yc-identity

/var/lib/yc/identity/tvm_client_secret:
  file.managed:
    - contents: 'YsYY6ebbVuFMrbJlr9ppYg' # TVM client secret for Identity *test* app
    - user: yc-identity
    - mode: '0400'
    - makedirs: True
    - replace: False
    - require:
      - yc_pkg: yc-identity

/var/lib/yc/identity/system_account_keys.json:
  file.managed:
    - source: salt://{{ slspath }}/sa_keys/{{ pillar['identity']['system_accounts']['public_keys_file'] }}
    - user: root
    - group: yc-identity
    - mode: '0040'
    - require:
      - yc_pkg: yc-identity

/var/lib/yc/identity/system_account.json:
  file.managed:
    - source: salt://{{ slspath }}/system_account.json
    - user: yc-identity
    - mode: '0400'
    - replace: False
    - require:
      - yc_pkg: yc-identity

populate-db:
  cmd.run:
    - name: '/usr/bin/yc-identity-populate-db'
    - require:
      - file: /etc/yc/identity/config.toml
      - file: /var/lib/yc/identity/system_account.json
    - onchanges:
      - yc_pkg: yc-identity

{%- if 'identity_private_tls' in lb_endpoints %}
yc-identity-private-tls:
  service.running:
    - name: nginx
    - require:
      - file: private_api_cert
      - file: private_api_key

private_api_cert:
  file.managed:
    - name: '/etc/ssl/certs/identity.private.api.pem'
    - follow_symlinks: False
    - source: '/etc/ssl/certs/ssl-cert-snakeoil.pem'
    - replace: False
    - user: root
    - mode: 644

private_api_key:
  file.managed:
    - name: '/etc/ssl/private/identity.private.api.key'
    - follow_symlinks: False
    - source: '/etc/ssl/private/ssl-cert-snakeoil.key'
    - replace: False
    - user: root
    - mode: 600

{%- set nginx_configs = ['identity-private-api.conf'] %}
{%- include 'nginx/install_configs.sls' %}

{%- endif %}

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}

{%- if grains["cluster_map"]["environment"] != "prod" %}
include:
  - nginx

{%- set nginx_configs = ['yc-serialws.conf'] %}
{%- include 'nginx/install_configs.sls' %}

{%- set nginx_certs = [ pillar['secrets']['serialws']['cert'], pillar['secrets']['serialws']['key'] ] %}
{%- include 'nginx/install_certs.sls' %}
{% endif -%}

yc-serialssh:
  yc_pkg.installed:
    - pkgs:
      - apparmor-ycloud-svc-serialssh-prof
      - yc-serialssh
  service.running:
    - enable: True
    - watch:
      - file: /etc/yc/serialssh/config.yaml
      - file: /etc/yc/serialssh/identity_key.json
      - file: /etc/yc/serialssh/ssh_host_rsa_key
      - file: /etc/yc/serialssh/ssh_host_rsa_key.pub
      - file: /etc/yc/serialssh/ssh_host_ed25519_key
      - file: /etc/yc/serialssh/ssh_host_ed25519_key.pub
      - yc_pkg: yc-serialssh

/etc/yc/serialssh/config.yaml:
  file.managed:
    - source: salt://{{ slspath }}/files/config.yaml
    - template: jinja
    - makedirs: True
    - require:
      - yc_pkg: yc-serialssh

/etc/yc/serialssh/identity_key.json:
  file.managed:
    - source: salt://{{ slspath }}/files/system_account.json
    - mode: '0040'
    - makedirs: True
    - replace: False
    - user: root
    - group: yc-serialssh
    - require:
      - yc_pkg: yc-serialssh

/etc/yc/serialssh/ssh_host_rsa_key:
  file.managed:
    - source: salt://{{ slspath }}/files/ssh_host_rsa_key
    - mode: '0040'
    - makedirs: True
    - replace: False
    - user: root
    - group: yc-serialssh
    - require:
      - yc_pkg: yc-serialssh

/etc/yc/serialssh/ssh_host_rsa_key.pub:
  file.managed:
    - source: salt://{{ slspath }}/files/ssh_host_rsa_key.pub
    - mode: '0444'
    - makedirs: True
    - replace: False
    - user: root
    - group: yc-serialssh
    - require:
      - yc_pkg: yc-serialssh

/etc/yc/serialssh/ssh_host_ed25519_key:
  file.managed:
    - source: salt://{{ slspath }}/files/ssh_host_ed25519_key
    - mode: '0040'
    - makedirs: True
    - replace: False
    - user: root
    - group: yc-serialssh
    - require:
      - yc_pkg: yc-serialssh

/etc/yc/serialssh/ssh_host_ed25519_key.pub:
  file.managed:
    - source: salt://{{ slspath }}/files/ssh_host_ed25519_key.pub
    - mode: '0444'
    - makedirs: True
    - replace: False
    - user: root
    - group: yc-serialssh
    - require:
      - yc_pkg: yc-serialssh

{%- from slspath+"/monitoring.yaml" import monitoring -%}
{%- include "common/deploy_mon_scripts.sls" %}

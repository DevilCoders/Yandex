{% set config_path = '/etc/yc-marketplace/solomon-agent.conf' %}

include:
  - common.repo

{{ config_path }}:
  file.managed:
    - makedirs: True
    - template: jinja
    - source: salt://{{ sls }}/files/solomon-agent.conf

/etc/systemd/system/solomon-agent.service:
  file.managed:
    - source: salt://{{ sls }}/files/solomon-agent.service
    - user: root
    - group: root
    - mode: 0644
    - template: jinja

{{ sls }}_package_solomon:
  pkg.installed:
    - name: yandex-solomon-agent-bin
    - version: "1:16.7" # CLOUD-14089

{{ sls }}_packages:
  pkg.latest:
    - pkgs:
      - python-requests
      - python-prometheus-client
      - yc-marketplace-solomon-plugins
    - refresh: True
    - require:
      - file: {{ config_path }}
      - cmd: apt-update

{{ sls }}:
  service.running:
    - enable: True
    - watch:
      - file: {{ config_path }}
    - require:
      - pkg: {{ sls }}_packages

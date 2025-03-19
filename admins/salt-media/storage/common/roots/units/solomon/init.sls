{% set unit = "solomon" %}
{% set push_service = pillar.get('push_service', 'all') %}
{% set push_project = pillar.get('push_project', 'noc') %}
{% set push_endpoint = pillar.get('push_endpoint', '/') %}

/etc/systemd/system/solomon.service:
  file.managed:
    - source: salt://files/{{ unit }}/solomon.service

solomon:
  service.running:
    - enable: True
  pkg.installed:
    - pkgs:
      - yandex-solomon-agent-bin

/etc/solomon.conf:
  file.managed:
    - source: salt://files/{{ unit }}/solomon.conf
    - template: jinja
    - context:
      push_service: {{push_service}}
      push_project: {{push_project}}
      push_endpoint: {{push_endpoint}}


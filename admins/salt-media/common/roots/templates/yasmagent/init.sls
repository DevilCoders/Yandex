{% set unit = 'yasmagent' %}
{% set config = pillar.get(unit) %}
/etc/default/yasmagent:
  file.managed:
    - source: salt://templates/yasmagent/files/yasmagent
    - mode: 755
    - user: root
    - group: root
    - template: jinja
    - context:
        config: {{ config|yaml }}

restart-yasmagent:
  pkg.installed:
    - pkgs:
      - yandex-yasmagent
  service.running:
    - name: yasmagent
    - enable: True
    - sig: yasmagent
    - watch:
      - file: /etc/default/yasmagent

{% set monrun_yasm = pillar.get('yasmagent-monrun', False) %}
{% if monrun_yasm %}
/usr/local/bin/monrun-yasm.py:
  file.managed:
    - source: salt://{{slspath}}/files/monrun-yasm.py
    - user: root
    - group: root
    - mode: 755

monrun_yasm:
  monrun.present:
    - name: "yasm-monitored"
    - command: /usr/local/bin/monrun-yasm.py
    - execution_interval: 300
{% endif %}

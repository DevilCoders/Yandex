{% from slspath + "/map.jinja" import galera with context %}
include:
  - .services

/usr/bin/galera_check.sh:
  file.managed:
    - name: /usr/bin/galera_check.sh
    - source: salt://templates/galera/files/galera_check.sh
    - template: jinja
    - makedirs: True
    - mode: '0755'
    - context:
        config: {{ galera }}

monrun_galera_check:
  monrun.present:
    - name: galera
    - type: galera
    - command: /usr/bin/galera_check.sh
    - execution_interval: 30
    - execution_timeout: 60

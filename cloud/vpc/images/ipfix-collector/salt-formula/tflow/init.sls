tflow:
  pkg.installed:
    - names: 
       - tflow
    - version: {{ salt['grains.get']('tflow:version') or '0.6ecd7ffb366d3602d1274b4e032a1c4ae5ed1696.trunk'}}


tflow-config:
  file.managed:
    - name: /etc/tflow/tflow.conf
    - source: salt://{{slspath}}/files/tflow.conf
    - makedirs: true
    - require:
      - pkg: tflow

tflow-service-file:
  file.managed:
    - name: /etc/systemd/system/tflow.service
    - source: salt://{{slspath}}/files/tflow.service
    - makedirs: true

tflow-service:
  service.enabled:
    - name: tflow
    - require:
      - file: tflow-service-file

tflow-logrotate:
  file.managed:
    - name: /etc/logrotate.d/tflow
    - source: salt://{{slspath}}/files/tflow.logrotate

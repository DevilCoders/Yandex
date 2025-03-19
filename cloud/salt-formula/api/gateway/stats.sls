# TODO: monitorings

/etc/systemd/system/statsd-exporter.service:
  file.managed:
    - source: salt://{{ slspath }}/files/stats/statsd-exporter.service
    - template: jinja

statsd-exporter:
  service.running:
    - enable: True
    - watch:
      - file: /etc/systemd/system/statsd-exporter.service
    - require:
      - yc_pkg: yc-api-gateway

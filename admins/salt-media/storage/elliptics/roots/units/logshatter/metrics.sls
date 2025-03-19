/usr/local/bin/clickhouse-custom-metrics:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/bin/clickhouse-custom-metrics
    - mode: 755

/usr/local/etc/clickhouse-custom-metrics.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/usr/local/etc/clickhouse-custom-metrics.conf
    - template: jinja
    - mode: 644

/etc/init/clickhouse-custom-metrics.conf:
  file.managed:
    - source: salt://{{ slspath }}/files/etc/init/clickhouse-custom-metrics.conf

clickhouse-custom-metrics:
  service:
    - dead
    - enable: False
    - watch:
      - file: /usr/local/etc/clickhouse-custom-metrics.conf

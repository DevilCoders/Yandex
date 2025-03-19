/etc/monrun/salt_solomon_proxy_status/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/sol_proxy/MANIFEST.json
    - makedirs: True
    - mode: 755

/etc/monrun/salt_solomon_proxy_status/sol_proxy_status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/sol_proxy/sol_proxy_status.sh
    - makedirs: True
    - mode: 755

/etc/monrun/salt_influx_proxy_status/MANIFEST.json:
  file.managed:
    - source: salt://{{ slspath }}/files/influx_proxy/MANIFEST.json
    - makedirs: True
    - mode: 755

/etc/monrun/salt_influx_proxy_status/influx_proxy_status.sh:
  file.managed:
    - source: salt://{{ slspath }}/files/influx_proxy/influx_proxy_status.sh
    - makedirs: True
    - mode: 755

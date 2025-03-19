{% set unit = 'sensors' %}

/etc/cron.d/sensors:
  file.managed:
    - source: salt://templates/sensors/sensors
    - mode: 644
    - user: root
    - group: root
    - makedirs: True

/etc/cron.yandex/sensors.sh:
  file.managed:
    - source: salt://templates/sensors/sensors.sh
    - mode: 755
    - user: root
    - group: root
    - makedirs: True

hddtemp:
  pkg:
    - installed

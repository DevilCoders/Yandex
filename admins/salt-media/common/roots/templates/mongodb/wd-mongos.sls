{% from slspath + '/map.jinja' import mongodb,cluster with context %}

/usr/bin/wd-mongos.sh:
  file.managed:
    - source: salt://templates/mongodb/files/usr/bin/wd-mongos.sh
    - user: root
    - group: root
    - mode: 775

/etc/cron.d/wd-mongos:
  {% if mongodb.mongos.watchdog %}
    file.managed:
      - source: salt://templates/mongodb/files/etc/cron.d/wd-mongos
      - user: root
      - group: root
      - mode: 644
  {% else %}
    file.absent
  {% endif %}


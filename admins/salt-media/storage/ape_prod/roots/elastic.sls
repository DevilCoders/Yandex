/etc/cron.d/elasticsearch_index_rotate:
  file.managed:
    - source: salt://elastic/cron.d/elasticsearch_index_rotate

/usr/share/elasticsearch/bin/index_pre-create.sh:
  file.managed:
    - source: salt://elastic/usr/share/elasticsearch/bin/index_pre-create.sh
    - mode: 755

/usr/share/elasticsearch/bin/index_rotate.sh:
  file.managed:
    - source: salt://elastic/usr/share/elasticsearch/bin/index_rotate.sh
    - mode: 755

/usr/local/bin/timetail_parser_elastic.py:
  file.managed:
    - source: salt://elastic/usr/local/bin/timetail_parser_elastic.py
    - mode: '0755'

/etc/yandex/loggiver/loggiver.pattern:
  file.managed:
    - source: salt://common-files/etc/yandex/loggiver/loggiver.pattern

/usr/share/elasticsearch/templates:
  file.recurse:
    - source: salt://elastic/usr/share/elasticsearch/templates

/etc/sysctl.d:
  file.recurse:
    - source: salt://elastic/sysctl.d

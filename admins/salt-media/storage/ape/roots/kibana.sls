/usr/local/bin/elasticsearch_index_rotate:
  file.managed:
    - source: salt://elastic/cron.d/elasticsearch_index_rotate

/usr/local/bin/:
  file.recurse:
    - source: salt://kibana/usr/local/bin/
    - file_mode: '0755'

/etc/monrun/conf.d:
  file.recurse:
    - source: salt://kibana/etc/monrun/conf.d/


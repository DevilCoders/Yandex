/etc/cron.d:
  file.recurse:
    - source: salt://zipkin/etc/cron.d/

/etc/zipkin-query-service/config.scala:
  file.recurse:
    - source: salt://zipkin/etc/zipkin-query-service/config.scala

/etc/nginx/sites-enabled:
  file.recurse:
    - source: salt://zipkin/etc/nginx/sites-enabled


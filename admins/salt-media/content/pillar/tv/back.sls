tls_tickets:
  keyname: rest-api.tv.{{"" if grains['yandex-environment'] in ['production', 'prestable'] else "tst."}}yandex.ru.key
  robot:  # default robot name is robot-media-salt
    srckeydir: tv_back
  monitoring:
    threshold_age: '30 hours ago'

nginx:
  lookup:
    log_params:
      name: 'access-log'
      access_log_name: access.log
      custom_fields: ["http_x_client_id=$http_x_client_id", "upstream_connect_time=$upstream_connect_time"]
    logrotate:
      frequency: hourly
      rotate: 168
      delaycompress: False
      compress: False

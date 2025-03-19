nginx:
  lookup:
    enabled: True
    vhost_default: False
    confd_params:
      access_log: 'off'
    events_params:
      worker_connections: '16384'
      multi_accept: 'on'
    main_params:
      worker_processes: '4'
      worker_rlimit_nofile: 65536
      working_directory: '/var/tmp'
    logrotate:
      frequency: daily
      rotate: 10
      delaycompress: False
      compress: False
    log_params:
      name: 'access-log-icecream'
      access_log_name: access.log

{% if grains['yandex-environment'] == 'production' %}
yav: {{ salt.yav.get('sec-01czttq3w42rdpm22re22ajnpp') | json }}
{% else %}
yav: {{ salt.yav.get('sec-01cztvm97x7mvv31pnt7svtgs6') | json }}
{% endif %}

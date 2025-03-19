nginx:
  lookup:
    enabled: True
    packages:
      - nginx
      - nginx-full
      - nginx-common
#    vhost_default: False
    vhost_default_config: salt://{{ slspath }}/files/etc/nginx/sites-enabled/00-default.conf

    listens:
      listen:
        ports:
          - '80'
          - '[::]:80'
      listen_ssl:
        ssl:
          ssl_certificate: /etc/nginx/ssl/forum.kinopoisk.ru.pem
          ssl_certificate_key: /etc/nginx/ssl/forum.kinopoisk.ru.pem
        ports:
          - '443'
          - '[::]:443'
#    accesslog_config: False
    confd_params:
      root: '/var/www-empty'

      gzip: 'on'

      tcp_nopush: 'on'
      tcp_nodelay: 'on'
      sendfile: 'off'

      client_header_timeout: '10m'
      client_body_timeout: '10m'
      send_timeout: '10m'

      client_max_body_size: '250m'
      connection_pool_size: 2048
      client_header_buffer_size: '4k'
      request_pool_size: '4k'
      large_client_header_buffers: '1000 8k'

      log_not_found: 'off'

      output_buffers: '128 64k'
      postpone_output: 1460

      keepalive_timeout: '75 60'
      ignore_invalid_headers: 'on'
      proxy_buffers: '8000 16k'

    events_params:
      worker_connections: 65536
      multi_accept: 'on'
    main_params:
      worker_processes: 3
      worker_rlimit_nofile: 100000
      error_log: '/var/log/nginx/error.log crit'
#    logrotate:
#      frequency: weekly
#      dateext: '-%Y%m%d-%s'
#      maxsize: 1G
#      rotate: 14



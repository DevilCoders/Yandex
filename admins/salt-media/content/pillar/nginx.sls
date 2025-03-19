nginx:
  lookup:
    enabled: True
    listens:
      listen:
        ports: ['80', '[::]:80']
    packages:
      - nginx=1.14.2-1.yandex.66
    confd_params:
      access_log: 'off'
      tcp_nopush: 'on'
      root: '/var/www-empty'
      server_name_in_redirect: 'off'
      keepalive_timeout: '60 60'
      keepalive_requests: '256'
      gzip_buffers: '64 16k'
      gzip_disable: 'msie6'
      client_header_timeout: '1m'
      client_body_timeout: '1m'
      send_timeout: '1m'
      reset_timedout_connection: 'on'
      client_header_buffer_size: '4k'
      large_client_header_buffers: '64 16k'
      client_max_body_size: '1024m'
      client_body_buffer_size: '50m'
      client_body_temp_path: '/var/tmp/nginx/client-temp 2 2'
      fastcgi_next_upstream: 'error timeout invalid_header'
      fastcgi_buffers: '64 16k'
      proxy_buffering:             'on'
      proxy_next_upstream:         'error timeout invalid_header'
      proxy_buffer_size:           '32k'
      proxy_buffers:               '64 16k'
      proxy_read_timeout: '3600s'
      include: '/etc/nginx/fastcgi_params'
      request_id_from_header: 'on'
    events_params:
      multi_accept: 'on'
    main_params:
      worker_rlimit_nofile: 65536
      working_directory: '/var/tmp'
    logrotate:
      frequency: daily
      maxsize: 1000M
      rotate: 7

nginx:
  lookup:
    enabled: True
    vhost_default: False
    confd_params:
      resolver: 'localhost'
      access_log: 'off'
      tcp_nopush: 'on'
      root: '/var/www-empty'
      reset_timedout_connection: 'on'
      client_header_buffer_size: '4k'
      large_client_header_buffers: '64 16k'
      # client_max_body_size: '1024m'
      client_body_buffer_size: '50m'
      client_body_temp_path: '/var/tmp/nginx/client-temp 2 2'
      fastcgi_next_upstream: 'error timeout invalid_header'
      fastcgi_buffers: '64 16k'
      proxy_next_upstream: 'error timeout invalid_header'
      proxy_buffer_size:           '32k'
      proxy_buffers:               '64 16k'
      include: '/etc/nginx/fastcgi_params'
      request_id_from_header: 'on'
    events_params:
      worker_connections: '65536'
      multi_accept: 'on'
    main_params:
      worker_rlimit_nofile: 65536
      working_directory: '/var/tmp'

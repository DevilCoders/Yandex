nginx:
  lookup:
    enabled: True
    service: nginx
    packages: [ nginx, nginx-full, nginx-common ]
    vhost_default: False
    listens: []
    accesslog_config: salt://templates/nginx/files/etc/nginx/conf.d/accesslog.conf
    confd_params:
      access_log: 'off'
      server_tokens: 'off'
      sendfile: 'on'
      tcp_nodelay: 'on'
      tcp_nopush: 'on'
      root: '/var/www-empty'
      default_type: 'application/octet-stream'
      server_name_in_redirect: 'off'
      keepalive_timeout: '60 60'
      keepalive_requests: '256'
      gzip: 'on'
      gzip_buffers: '64 16k'
      gzip_comp_level: '9'
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
    events_params:
      use: 'epoll'
      worker_connections: '16384'
      multi_accept: 'on'
    main_params:
      worker_processes: '12'
      pid: '/var/run/nginx.pid'
      user: 'www-data'
      worker_rlimit_nofile: 65536
      working_directory: '/var/tmp'
    logrotate:
      maxsize: 500M
      rotate: 10
      dateext: '-%Y%m%d-%s'
      delaycompress: False
      compress: False

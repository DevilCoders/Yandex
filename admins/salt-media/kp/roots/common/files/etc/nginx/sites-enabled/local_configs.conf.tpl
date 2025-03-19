upstream {{ upstream_name  }} {
  keepalive 16;
  server {{ server  }}:{{ server_port  }};
}

server {
  listen {{ port  }};
  listen [::]:{{ port  }};

  keepalive_requests 1000;
  keepalive_timeout 120 60;

  tskv_log /var/log/nginx/{{ logname  }} {{ logtype  }};

  location / {
    proxy_pass             {{ schema  }}://{{ upstream_name  }};
    proxy_read_timeout     {{ proxy_read_timeout  }}ms;
    proxy_connect_timeout  {{ proxy_connect_timeout  }}ms;

    proxy_http_version 1.1;

    proxy_set_header Host {{ server  }};
    proxy_set_header Connection "";
  }
}


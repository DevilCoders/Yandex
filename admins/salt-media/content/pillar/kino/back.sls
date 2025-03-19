tls_tickets:
  keyname: rest-api.tv.{{"" if grains['yandex-environment'] in ['production', 'prestable'] else "tst."}}yandex.ru.key
  robot:  # default robot name is robot-media-salt
    srckeydir: kino_back
  monitoring:
    threshold_age: '30 hours ago'

nginx:
  lookup:
    log_params:
      name: 'access-log'
      access_log_name: access.log
    confd_params:
      access_log: '/var/log/nginx/access_post.log postformat'
    confd_addons:
      "02-accesslog_post": |
        log_format postformat '[$time_local] $http_host $remote_addr "$request" '
                              '$status "$http_referer" "$http_user_agent" '
                              '"$http_cookie" $request_time $upstream_cache_status '
                              '$bytes_sent "$upstream_addr" "$upstream_status" '
                              '"$upstream_response_time" "$request_body"';

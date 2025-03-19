nginx_configs:
  upstreams:

    kp1-api-internal:
      production:
        server: 'kp1-api-internal.kp.yandex.net'
      prestable:
        server: 'kp1-api-internal.kp.yandex.net'
      default:
        server: 'kp1-api-internal.tst.kp.yandex.net'
        logtype: 'kinopoisk-tskv-backend-log'
        logname: 'kp1-api-internal'
        proxy_read_timeout: 300
        proxy_connect_timeout: 200
        port: 9494
        server_port: 443
        schema: https

    awaps:
      default:
        logtype: 'kinopoisk-tskv-backend-log'
        server: 'awaps.yandex.ru'
        server_port: 80
        port: 9898
        logname: 'awaps'
        proxy_read_timeout: 300
        proxy_connect_timeout: 200
        schema: http

    kp1-media-api:
      stress:
        server: 'kp1-media-api.stress.kp.yandex.net'
      production:
        server: 'kp1-media-api.kp.yandex.net'
      prestable:
        server: 'kp1-media-api.kp.yandex.net'
      default:
        server: 'kp1-media-api.tst.kp.yandex.net'
        logtype: 'kinopoisk-tskv-backend-log'
        server_port: 443
        port: 9999
        logname: 'kp1-media-api'
        proxy_read_timeout: 1500
        proxy_connect_timeout: 200
        schema: https

    laas:
      default:
        logtype: 'kinopoisk-tskv-backend-log'
        server: 'laas.yandex.ru'
        server_port: 80
        port: 9696
        logname: 'laas'
        proxy_read_timeout: 300
        proxy_connect_timeout: 200
        schema: http

    music-api:
      default:
        server: 'api.music.yandex.net'
        server_port: 443
        logtype: 'kinopoisk-tskv-backend-log'
        port: 9595
        logname: 'music-api'
        proxy_read_timeout: 300
        proxy_connect_timeout: 200
        schema: https


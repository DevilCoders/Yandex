include:
  - templates.certificates
  - units.tune_tun
  - units.iface-ip-conf


monrun-nginx-check:
  monrun.present:
    - name: nginx
    - command: '/usr/bin/http_check.sh ping 80'
    - execution_interval: 60


/etc/nginx/headers-cors-credentials.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        # https://st.yandex-team.ru/VH-1645#1495742398000
        more_clear_headers "Access-Control-Allow-Origin";
        more_set_headers "Access-Control-Allow-Origin: $allowed_origin";

        more_clear_headers "Access-Control-Allow-Credentials";
        more_set_headers "Access-Control-Allow-Credentials: true";

        more_clear_headers "Access-Control-Allow-Headers";
        more_set_headers "Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept, Range";

        # https://st.yandex-team.ru/VH-3832
        more_clear_headers "Timing-Allow-Origin";
        more_set_headers "Timing-Allow-Origin: $allowed_origin";


/etc/nginx/map_allowed_origins.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        #
        # for strm-streams (/dvr and /kal)
        #
        "https://yastatic.net" $http_origin;
        "~https://.*\.yastatic.net" $http_origin;
        "~*.yandex.ru(:[0-9]+)?" $http_origin;
        "~*.yandex.net" $http_origin;
        "~*.yandex-team.ru" $http_origin;
        "~localhost(:[0-9]+)?" $http_origin;

        #
        # for OTT - https://st.yandex-team.ru/VH-2685
        #
        "~https?://.*kinopoisk\.ru" $http_origin;
        "~https?://.*smarttv\.ott\.yandex.ru" $http_origin;

        #
        # for 1tv.ru by toshik@
        #
        "https://www.1tv.ru" $http_origin;
        "~https://.*\.1tv.ru" $http_origin;

        #
        # tmp allowed shaka player - to test widevine drm - https://st.yandex-team.ru/VH-2995
        #
        "~https?://shaka-player-demo\.appspot\.com" $http_origin;


/etc/nginx/sites-enabled/chunks-eater.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 644
    - contents: |
        map $http_origin $allowed_origin {
                include map_allowed_origins.conf;
        }



        server {
                listen 80;
                listen 443 ssl;
                listen [::]:80;
                listen [::]:443 ssl;
                server_name log.strm.yandex.net;

                ssl_certificate /etc/nginx/ssl/strm.yandex.net.pem;
                ssl_certificate_key /etc/nginx/ssl/strm.yandex.net.key;
                ssl_prefer_server_ciphers on;
                ssl_protocols SSLv3 TLSv1 TLSv1.1 TLSv1.2;
                ssl_ciphers AES128+RSA:RC4-SHA:kRSA:DES-CBC3-SHA:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2;

                client_max_body_size 20m;


                location / {
                        include headers-cors-credentials.conf;
                        proxy_pass http://127.0.0.1:8080;
                        proxy_set_header Host $host;
                        proxy_http_version 1.1;
                        proxy_set_header        X-Request-Id    $request_id;
                        proxy_set_header Connection "";
                        proxy_set_header X-Real-IP $remote_addr;
                        proxy_read_timeout 10s;
                        request_id_from_header on;
                        request_id_header_name X-Request-Id;
                }
        }

/etc/cron.daily/strm-chunks-eater-cleanup:
  file.managed:
    - user: root
    - group: root
    - mode: 755
    - contents: |
        #!/bin/bash
        /usr/bin/strm-chunks-eater -c /etc/yandex/strm-chunks-eater.yaml --store-days 2 --cleanup 2>> /var/log/strm-chunks-eater-cleanup.log

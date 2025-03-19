data:
    dataproc-ui-proxy:
        config:
            disable_auth: false
            expose_error_details: false
            authorized_key:
                id: aje1a62aghb85vp3e9u4
                service_account_id: ajet1pgshfjc37uue3b2
                private_key: |
                    {{ salt.yav.get('ver-01eezf0nw8ze8jzek4q3mkz4wy[private_key]') | indent(20) }}
            user_auth:
                host: https://dataproc-ui.yandexcloud.net
                callback_path: /oauth2
                session_cookie_auth_key: {{ salt.yav.get('ver-01eezg0gwbfvp9s63jryj1akey[authkey]') }}
                oauth:
                    clientid: dataproc-ui-proxy
                    clientsecret: {{ salt.yav.get('ver-01ej683vhgwpdejn02y1qeyywt[secret]') }}
                    scopes:
                        - openid
                    endpoint:
                        authurl: https://auth.cloud.yandex.ru/oauth/authorize
                        tokenurl: https://auth.cloud.yandex.ru/oauth/token
            token_service:
                addr: ts.private-api.cloud.yandex.net:4282
            session_service:
                addr: ss.private-api.cloud.yandex.net:8655
            access_service:
                addr: as.private-api.cloud.yandex.net:4286
            internal_api:
                url: https://mdb.private-api.cloud.yandex.net
                access_id: {{ salt.yav.get('ver-01edxck0hfncb83qcmdz70a0xn[id]') }}
                access_secret: {{ salt.yav.get('ver-01edxck0hfncb83qcmdz70a0xn[secret]') }}
            http_server:
                listen_addr: ":443"
                tls:
                    cert: |
                        {{ salt.yav.get('sec-01eezhn6f248tz9p7hfdbk39kg[cert]') | indent(24) }}
                    key: |
                        {{ salt.yav.get('sec-01eezhn6f248tz9p7hfdbk39kg[key]') | indent(24) }}
            proxy:
                csp:
                    enabled: true
                    report_uri: https://csp.yandex.net/csp?from=dataproc-ui.yandexcloud.net&project=cloud
                    block: false
                dataplane_domain: mdb.yandexcloud.net
                ui_proxy_domain: dataproc-ui.yandexcloud.net
                service_by_port:
                    9870: hdfs
                    9864: webhdfs

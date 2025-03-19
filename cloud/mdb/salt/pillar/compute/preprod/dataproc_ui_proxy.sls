data:
    dataproc-ui-proxy:
        config:
            disable_auth: false
            expose_error_details: true
            authorized_key:
                id: bfbrrldioe2mhhdec278
                service_account_id: bfb3prdhe9vr858c6o8j
                private_key: |
                    {{ salt.yav.get('ver-01eezfcwwx0b09nrgyn8ge75fs[private_key]') | indent(20) }}
            user_auth:
                host: https://dataproc-ui.cloud-preprod.yandex.net
                callback_path: /oauth2
                session_cookie_auth_key: {{ salt.yav.get('ver-01e5cdcv0s3ypfy5chs6753n64[authkey]') }}
                oauth:
                    clientid: dataproc-ui-proxy
                    clientsecret: {{ salt.yav.get('ver-01e5mfrv3cp0q7gkjbg33d6dr9[secret]') }}
                    scopes:
                        - openid
                    endpoint:
                        authurl: https://auth-preprod.cloud.yandex.ru/oauth/authorize
                        tokenurl: https://auth-preprod.cloud.yandex.ru/oauth/token
            token_service:
                addr: ts.private-api.cloud-preprod.yandex.net:4282
            session_service:
                addr: ss.private-api.cloud-preprod.yandex.net:8655
            access_service:
                addr: as.private-api.cloud-preprod.yandex.net:4286
            internal_api:
                url: https://mdb.private-api.cloud-preprod.yandex.net
                access_id: {{ salt.yav.get('ver-01edxbx92r3cwnag1bnj3mhnd3[id]') }}
                access_secret: {{ salt.yav.get('ver-01edxbx92r3cwnag1bnj3mhnd3[secret]') }}
            http_server:
                listen_addr: ":443"
                tls:
                    cert: |
                        {{ salt.yav.get('sec-01eah70dq2xdjb8as4xhtpgg6j[cert]') | indent(24) }}
                    key: |
                        {{ salt.yav.get('sec-01eah70dq2xdjb8as4xhtpgg6j[key]') | indent(24) }}
            proxy:
                csp:
                    enabled: true
                    report_uri: https://csp.yandex.net/csp?from=dataproc-ui.cloud-preprod.yandex.net&project=cloud
                    block: false
                dataplane_domain: mdb.cloud-preprod.yandex.net
                ui_proxy_domain: dataproc-ui.cloud-preprod.yandex.net
                service_by_port:
                    9870: hdfs
                    9864: webhdfs

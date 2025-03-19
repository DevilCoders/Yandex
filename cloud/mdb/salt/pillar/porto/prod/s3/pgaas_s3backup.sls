data:
    s3:
        endpoint: "https+path://s3.mds.yandex.net"
        host: s3.mds.yandex.net
        access_key_id: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[id]') }}
        access_secret_key: {{ salt.yav.get('ver-01fvtqb12bqr6r8brn8g369p8f[secret]') }}
        user_id: 1120000000045965
        service_id: 1373

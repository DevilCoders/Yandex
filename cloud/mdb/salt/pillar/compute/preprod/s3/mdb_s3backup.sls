data:
    s3:
        endpoint: https://s3-private.mds.yandex.net
        host: s3-private.mds.yandex.net
        access_key_id: {{ salt.yav.get('ver-01dwh3a0jxezxewqgd2ywwfng7[id]') }}
        access_secret_key: {{ salt.yav.get('ver-01dwh3a0jxezxewqgd2ywwfng7[key]') }}
        user_id: 1120000000048623
        service_id: 1895
        region: us-east-1
        virtual_addressing_style: True

data:
    tvmtool:
        config:
            client: mdb-gerrit-sso
            secret: {{ salt.yav.get('ver-01e9gdgkv08sbsx6bvhm19f9hr[secret]') }}
        port: 50001
        token: {{ salt.yav.get('ver-01e9gdgkv08sbsx6bvhm19f9hr[token]') }}
        tvm_id: 2011256
    gerrit:
        server_name: review.db.yandex-team.ru

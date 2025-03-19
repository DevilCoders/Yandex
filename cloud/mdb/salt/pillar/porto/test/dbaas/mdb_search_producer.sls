data:
    tvmtool:
        token: {{ salt.yav.get('ver-01dvxcjv2t79avsqs526t7vyyz[token]') }}
        config:
            clients:
                search-producer:
                    secret: {{ salt.yav.get('ver-01dvxcjv2t79avsqs526t7vyyz[secret]') }}

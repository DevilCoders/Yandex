data:
    tvmtool:
        token: {{ salt.yav.get('ver-01e07jrmkz17peyyashh3hwfj8[token]') }}
        config:
            clients:
                search-producer:
                    secret: {{ salt.yav.get('ver-01e07jrmkz17peyyashh3hwfj8[secret]') }}


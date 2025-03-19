data:
    tvmtool:
        token: {{ salt.yav.get('ver-01dws8cv6nx16svjh890x225ry[token]') }}
        config:
            clients:
                search-producer:
                    secret: {{ salt.yav.get('ver-01dws8cv6nx16svjh890x225ry[secret]') }}


data:
    sentry:
        database:
            password: {{ salt.yav.get('ver-01e0cpnm7pnph4bfdmew9ajh5s[password]') }}
        cache:
            password: {{ salt.yav.get('ver-01esgqbtvgezv4s1jp294wa00p[password]') }}
        secret_key: {{ salt.yav.get('ver-01e0cpx43aeghrj0q8v0tvkc83[secret]') }}
        s3:
            access_key: {{ salt.yav.get('ver-01etaqgkm4a9b0qg2bkneab9rm[access_key]') }}
            secret_key: {{ salt.yav.get('ver-01etaqgkm4a9b0qg2bkneab9rm[secret_key]') }}

data:
    dbaas_pillar:
        urls:
            - 'https://mdb.private-api.cloud-preprod.yandex.net/api/v1.0/config/'
        access_id: {{ salt.yav.get('ver-01dw7qkq5wt078nf0gf3s8kzss[id]') }}
        access_secret: {{ salt.yav.get('ver-01dw7qkq5wt078nf0gf3s8kzss[secret]') }}
        api_pub_key: {{ salt.yav.get('ver-01dwh5tptt85r66reg4f3vdcay[public]') }}
        salt_sec_key: {{ salt.yav.get('ver-01dwh5tptt85r66reg4f3vdcay[secret]') }}
    mdb_secrets:
        salt_private_key: {{ salt.yav.get('ver-01dwh7dt5xdn1jch986qdkmnc6[private]') }}
        public_key: {{ salt.yav.get('ver-01dwh7dt5xdn1jch986qdkmnc6[public]') }}
        use_iam_tokens: true
        sa_private_key: |
            {{ salt.yav.get('ver-01efp5s7jrjry8kke82fz521qq[private_key]') | indent(12) }}
        sa_id: 'bfbq2iub7j14rlk4ttus'
        sa_key_id: {{ salt.yav.get('ver-01efp5s7jrjry8kke82fz521qq[id]') }}
        iam_hostname: 'iam.api.cloud-preprod.yandex.net'
        tokens_url: 'https://iam.api.cloud-preprod.yandex.net/iam/v1/tokens'
        cert_url: 'https://mdb-secrets.private-api.cloud-preprod.yandex.net/v1/cert'
        gpg_url: 'https://mdb-secrets.private-api.cloud-preprod.yandex.net/v1/gpg'

data:
    db_maintenance:
        use_tvm: true
        tvm_client_id: 2010948
        tvm_secret: {{ salt.yav.get('ver-01e9e3s57fkna5mmheq2nxaxmt[secret]') }}
        pgaas_api_token: {{ salt.yav.get('ver-01g36bbq5z2kk40ep2d66eb9c0[token]') }}
        deploy_api_v2_token: {{ salt.yav.get('ver-01dz8pb27s7tpg298ava8s8tdy[token]') }}
        ssl:
            cert: |
                {{ salt.yav.get('ver-01e9e3gkj5v6fs8bep5c3fvmha[cert]') | indent(16) }}
            key: |
                {{ salt.yav.get('ver-01e9e3gkj5v6fs8bep5c3fvmha[private]') | indent(16) }}

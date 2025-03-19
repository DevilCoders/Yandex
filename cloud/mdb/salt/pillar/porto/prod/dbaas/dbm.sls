data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e1xwe04e6e7q8vwv7ynnh1rx[secret]') | indent(12) }}
    db_maintenance:
        use_tvm: true
        tvm_client_id: 2010948
        tvm_secret: {{ salt.yav.get('ver-01e9e3s57fkna5mmheq2nxaxmt[secret]') }}
        deploy_api_v2_token: {{ salt.yav.get('ver-01dz8pb27s7tpg298ava8s8tdy[token]') }}

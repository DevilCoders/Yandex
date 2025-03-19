data:
    common:
        dh: |
            {{ salt.yav.get('ver-01e1xwe04e6e7q8vwv7ynnh1rx[secret]') | indent(12) }}
    db_maintenance:
        tvm_client_id: 2009984
        tvm_secret: {{ salt.yav.get('ver-01e1xwx6s3whthbmtx060wv1s9[secret]') }}
        pgaas_api_token: {{ salt.yav.get('ver-01g36bbjbgmn2v31gqk3c8gkrg[token]') }}
        deploy_api_v2_token: {{ salt.yav.get('ver-01dz8pb27s7tpg298ava8s8tdy[token]') }}

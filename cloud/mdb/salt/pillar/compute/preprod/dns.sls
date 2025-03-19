data:
    common:
        dh: |
            {{ salt.yav.get('ver-01dws3rd9ab6vxgkk4g9qtdaj0[private]') | indent(12) }}
    dnsapi:
        slayer:
            baseurl: "https://dns-api.yandex.net"
            account: robot-dnsapi-mdb
            token: {{ salt.yav.get('ver-01dw7mpmcc77axtrf8pm9a5kzq[token2]') }}
            fqdnsuffix: mdb.cloud-preprod.yandex.net
        compute:
            grpcurl: network-api-internal.private-api.cloud-preprod.yandex.net:9823
            token: {{ salt.yav.get('ver-01dws428xf9svn07yh618bpbxr[token]') }}
            fqdnsuffix: mdb.cloud-preprod.yandex.net
        service_account:
            id: {{ salt.yav.get('ver-01er49bx4me81yg62jqczy7ahz[id]') }}
            key_id: {{ salt.yav.get('ver-01er49bx4me81yg62jqczy7ahz[key_id]') }}
            private_key: {{ salt.yav.get('ver-01er49bx4me81yg62jqczy7ahz[private_key]') | yaml_encode }}
    secretsstore:
        postgresql:
            hosts:
                - meta-dbaas-preprod01f.cloud-preprod.yandex.net:6432
                - meta-dbaas-preprod01h.cloud-preprod.yandex.net:6432
                - meta-dbaas-preprod01k.cloud-preprod.yandex.net:6432
            db: dbaas_metadb
            user: mdb_dns
            password: {{ salt.yav.get('ver-01dws46m19dkyhhmmmgg2dz0m5[password]') }}
    mdb-dns:
        server_name: mdb-dns.private-api.cloud-preprod.yandex.net
        tls_key: |
            {{ salt.yav.get('ver-01dws4abqzpc3tjbd027ve5mqh[private]') | indent(12) }}
        tls_crt: |
            {{ salt.yav.get('ver-01dws4abqzpc3tjbd027ve5mqh[cert]') | indent(12) }}

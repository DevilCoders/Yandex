data:
    image-releaser:
        mode: "porto"
        stable_duration: "4h"
        porto:
            source_bucket: 'dbaas-images-vm-built-test'
            destination_bucket: 'dbaas-images-vm-built'
            s3:
                host: "s3.mds.yandex.net"
                access_key: {{ salt.yav.get('ver-01e0q7a02kvyye90ces5zt0yte[id]') }}
                secret_key: {{ salt.yav.get('ver-01e0q7a02kvyye90ces5zt0yte[secret]') }}
        sentry:
            dsn: {{ salt.yav.get('ver-01e49s0rwn57m5g6h2mrzv8p77[dsn]') }}
            environment: "porto-prod"

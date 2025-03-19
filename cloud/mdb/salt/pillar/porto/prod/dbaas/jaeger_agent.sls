data:
    jaeger-agent:
        tls: False
        config:
            reporter:
                grpc:
                    host-port: mdb-prod.c.jaeger.yandex-team.ru:14250

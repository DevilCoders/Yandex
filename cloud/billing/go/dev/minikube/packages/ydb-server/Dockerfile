FROM registry.yandex.net/ubuntu:bionic

WORKDIR /

RUN mkdir -p /Berkanavt/kikimr/cfg
COPY kikimr /Berkanavt/kikimr/bin/kikimr
COPY lbk_recipe /lbk_recipe
COPY ydb /ydb

RUN chmod +x /lbk_recipe

ENV YDB_YQL_SYNTAX_VERSION=1

# 2135: grpc
EXPOSE 2135
HEALTHCHECK --interval=1s CMD /ydb -e localhost:${GRPC_PORT-2135} -d /Root scheme ls /Root/PQ
CMD /lbk_recipe start --ydb-working-dir /Berkanavt/kikimr/cfg --ydb-binary-path /Berkanavt/kikimr/bin/kikimr --fixed-ports --topic test-topic -- && tail -f /dev/null

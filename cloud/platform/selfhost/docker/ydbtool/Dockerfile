FROM registry.yandex.net/cloud/api/ydb:stable as ydb

FROM ubuntu:bionic

COPY --from=ydb /ydb /ydb

CMD ["/ydb", "--help"]


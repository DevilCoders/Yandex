#Build combined image using yandex base image with out self-singed cert and temporal auto-setup
ARG TEMPORAL_VERSION

FROM registry.yandex.net/rtc-base/focal:stable

FROM temporalio/auto-setup:${TEMPORAL_VERSION} as auto-setup

#TODO remove after release with https://github.com/temporalio/temporal/commit/d2f41f0502d7afe9e7dacc770413c1ac63df70eb
USER root
RUN sed -i -e 's/temporal-sql-tool --plugin postgres --ep "${POSTGRES_SEEDS}" -u "${POSTGRES_USER}" -p "${DB_PORT}" create --db "${DBNAME}"/ls/g' auto-setup.sh && \
    sed -i -e 's/temporal-sql-tool --plugin postgres --ep "${POSTGRES_SEEDS}" -u "${POSTGRES_USER}" -p "${DB_PORT}" create --db "${VISIBILITY_DBNAME}"/ls/g' auto-setup.sh


#TODO разобратсья почему сертификат не подтянулся из базового яндекс образа
RUN sed -i -e 's/curl/curl -k/g' auto-setup.sh



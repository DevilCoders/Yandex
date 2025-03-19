[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+logbroker+partitionmax), [Алерт в Juggler](https://juggler.yandex-team.ru/check_details/?host=yc_compute_logbroker_prod&service=topic-compute-journald)

## Описание
Мониторинг средних значений потребления квоты на запись в партицию за минуту по всем партициям топика.

## Подробности
Алерт сейчас временно замьючен. Для нас ситуация упирания в квоту на запись по одной партиции - нормальна, т.к. мы пишем непрерывно, без использования записи батчами. 

## Ссылки
- [Документация по метрике PartitionMaxWriteQuotaUsage](https://logbroker.yandex-team.ru/docs/reference/metrics#PartitionMaxWriteQuotaUsage)
- [UI LogBroker'a](https://logbroker.cloud.yandex.ru/yc-logbroker/accounts/yc.compute.cloud/logs?page=browser&type=directory)
- [Дашборд Compute LogBroker'a в Графане](https://grafana.yandex-team.ru/d/LFsrF59Mk/logbroker?orgId=1&refresh=5s&from=now-3h&to=now&var-cluster=preprod&var-topic=logs%2Fjournald%2Ftopic&var-account=yc.compute.cloud)

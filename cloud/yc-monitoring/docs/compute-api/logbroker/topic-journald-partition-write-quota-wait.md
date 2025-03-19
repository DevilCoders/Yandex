[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+logbroker+partitionw), [Алерт в Juggler](https://juggler.yandex-team.ru/check_details/?host=yc_compute_logbroker_prod&service=topic-compute-journald)

## Описание
Мониторинг троттлинга (замедления) записи в одну из партиций топика LogBroker'ом

## Подробности
В определенную партицию топика пишут слишком много, LB притормаживает запись в неё.

## Диагностика
- Смотрим на [дашборде](https://grafana.yandex-team.ru/d/LFsrF59Mk/logbroker?orgId=1&refresh=5s&from=now-24h&to=now&var-cluster=preprod&var-topic=logs%2Fjournald%2Ftopic&var-account=yc.compute.cloud) (не забыть выбрать prod/preprod) график `Throttling on partitions`. Видим всплески, запоминаем их время.
- Идём на [дашборд Compute логов](https://grafana.yandex-team.ru/d/7H16_F6Mz/cloud-compute-logs?orgId=1), перебираем все zone'ы и ищем всплески на графике `Most used partitions`. Запоминаем партицию
- В селекторе сверху выбираем партицию и смотрим на график `Writers` - видим инстанс, с которого идёт обильная запись
- Идём на инстанс и смотрим какой процесс шалит, например так: `journalctl -o json-pretty -S ’10:20’ -U ’10:30’ | grep UNIT | sort | uniq -c`

## Ссылки
- [Документация по метрике PartitionWriteQuotaWaitOriginal](https://logbroker.yandex-team.ru/docs/reference/metrics#PartitionWriteQuotaWaitOriginal)
- [UI LogBroker'a](https://logbroker.cloud.yandex.ru/yc-logbroker/accounts/yc.compute.cloud/logs?page=browser&type=directory)
- [Дашборд Compute LogBroker'a в Графане](https://grafana.yandex-team.ru/d/LFsrF59Mk/logbroker?orgId=1&refresh=5s&from=now-3h&to=now&var-cluster=preprod&var-topic=logs%2Fjournald%2Ftopic&var-account=yc.compute.cloud)
- [Дашборд Compute логов в Графане](https://grafana.yandex-team.ru/d/7H16_F6Mz/cloud-compute-logs?orgId=1&var-cluster=prod&var-zone=myt&var-partition=-)

[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+logbroker+Read), [Алерт в Juggler](https://juggler.yandex-team.ru/check_details/?host=yc_compute_logbroker_prod&service=topic-compute-journald)

## Описание
Мониторинг времени прошедшего между последней записью и чтением из нашего топика в LB по всем партициям

## Подробности
Горит по тем же причинам, что **topic-journald-messages-read**, т.к. у нас всего один топик => остановилось чтение.

## Диагностика
- Смотрим на [дашборде](https://grafana.yandex-team.ru/d/LFsrF59Mk/logbroker?orgId=1&refresh=5s&from=now-24h&to=now&var-cluster=preprod&var-topic=logs%2Fjournald%2Ftopic&var-account=yc.compute.cloud) (не забыть выбрать prod/preprod) график `Read time lag`. Если монотонно растёт - значит есть проблема.
- Не лишним на [том же дашборде](https://grafana.yandex-team.ru/d/LFsrF59Mk/logbroker?orgId=1&refresh=5s&from=now-24h&to=now&var-cluster=preprod&var-topic=logs%2Fjournald%2Ftopic&var-account=yc.compute.cloud) посмотреть на `Messages read` и увидеть там 0.
- Т.к. логи отправляются со всех машинок - заходим на любую и дёргаем дебажную ручку yc-log-reader'a: `curl -s localhost:6180/debug` и получить информацию о состоянии буфера
- С собранной информацией идём к дежурному LB

## Ссылки
- [Документация по метрике ReadTimeLagMs](https://logbroker.yandex-team.ru/docs/reference/metrics#ReadTimeLagMs)
- [UI LogBroker'a](https://logbroker.cloud.yandex.ru/yc-logbroker/accounts/yc.compute.cloud/logs?page=browser&type=directory)
- [Дашборд Compute LogBroker'a в Графане](https://grafana.yandex-team.ru/d/LFsrF59Mk/logbroker?orgId=1&refresh=5s&from=now-3h&to=now&var-cluster=preprod&var-topic=logs%2Fjournald%2Ftopic&var-account=yc.compute.cloud)

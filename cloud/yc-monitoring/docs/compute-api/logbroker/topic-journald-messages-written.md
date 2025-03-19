[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+logbroker+messageswritten), [Алерт в Juggler](https://juggler.yandex-team.ru/check_details/?host=yc_compute_logbroker_prod&service=topic-compute-journald)

## Описание
Мониторинг записи логов в топик в LB

## Подробности
Мониторинг записи данных (сообщений в секунду) в исходный топик в LB. Если опускается до 0 - запись остановилась. Вероятнее всего, проблема в yc-log-reader'e.

## Диагностика
- Смотрим на [дашборде](https://grafana.yandex-team.ru/d/LFsrF59Mk/logbroker?orgId=1&refresh=5s&from=now-24h&to=now&var-cluster=preprod&var-topic=logs%2Fjournald%2Ftopic&var-account=yc.compute.cloud) (не забыть выбрать prod/preprod) график `Messages written`. Если есть просадка в 0 - значит есть проблема.
- Т.к. логи отправляются со всех машинок - заходим на любую и смотрим ошибки  в `/var/log/yc-log-reader/*.log`
- На тех же машинках можно дёрнуть дебажную ручку yc-log-reader'a: `curl -s localhost:6180/debug` и получить информацию о состоянии буфера чтения
- С собранной информацией идём к дежурному LB

## Ссылки
- [Документация по метрике MessagesWrittenOriginal](https://logbroker.yandex-team.ru/docs/reference/metrics#MessagesWrittenOriginal)
- [UI LogBroker'a](https://logbroker.cloud.yandex.ru/yc-logbroker/accounts/yc.compute.cloud/logs?page=browser&type=directory)
- [Дашборд Compute LogBroker'a в Графане](https://grafana.yandex-team.ru/d/LFsrF59Mk/logbroker?orgId=1&refresh=5s&from=now-3h&to=now&var-cluster=preprod&var-topic=logs%2Fjournald%2Ftopic&var-account=yc.compute.cloud)

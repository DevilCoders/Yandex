[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=idempotence), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-idempotence-tokens-age)

## Описание
Мониторинг таски очистки завершённых операций

## Подробности
Загорается при увеличении отставания в очистке завершённых операций

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?viewPanel=6&orgId=1&from=now-30d&to=now)
- Проверить gc-таски на голове `sudo yc-compute-tasksctl gc-operations list`
- Если тасок нет - разобраться куда они делись и стартануть: `sudo yc-compute-tasksctl gc-operations start`

## Ссылки
- Дашборд в [Графане](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1&from=now-30d&to=now)

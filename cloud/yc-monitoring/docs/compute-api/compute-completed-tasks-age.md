[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=idempotence), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-idempotence-tokens-age)

## Описание
Мониторинг таски очистки завершённых тасков

## Подробности
Загорается при увеличении отставания в очистке завершённых тасков

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?viewPanel=14&orgId=1&from=now-30d&to=now)
- Проверить gc-таски на голове `sudo yc-compute-tasksctl gc-tasks list`
- Если тасок нет - разобраться куда они делись и стартануть: `sudo yc-compute-tasksctl gc-tasks start`

## Ссылки
- Дашборд в [Графане](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1&from=now-30d&to=now)

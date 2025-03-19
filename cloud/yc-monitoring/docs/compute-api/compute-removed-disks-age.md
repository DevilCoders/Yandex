[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=idempotence), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-idempotence-tokens-age)

## Описание
Мониторинг таски очистки удалённых дисков

## Подробности
Загорается при увеличении отставания в очистке удалённых дисков

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/9YHmNYcWz/cloud-compute-disks?viewPanel=3&orgId=1)
- Проверить gc-таски на голове `sudo yc-compute-tasksctl gc-disks list`
- Если тасок нет - разобраться куда они делись и стартануть: `sudo yc-compute-tasksctl gc-disks start`

## Ссылки
- Дашборд в [Графане](https://grafana.yandex-team.ru/d/9YHmNYcWz/cloud-compute-disks?orgId=1)

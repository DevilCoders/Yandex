[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=index-), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-search-index-*)

## Описание
Мониторинг тасок переиндексации поисковых event'ов

## Подробности
Загорается при увеличении отставания в построении поискового индекса для дисков и инстансов.

## Диагностика
- Посмотреть на [графики](https://grafana.yandex-team.ru/d/AKp0gx9Gz/compute-index-tasks?orgId=1&refresh=5s&from=now-7d&to=now)
- Проверить операции на голове `sudo yc-compute-tasksctl tasks list-processing` (смотреть на таски вида `index-*/search-*`)
- Читать логи по операциям с ошибками `sudo journalctl TASK_ID=<id>`

## Ссылки
- [Дашборд в Графане](https://grafana.yandex-team.ru/d/AKp0gx9Gz/compute-index-tasks?orgId=1&refresh=5s&from=now-7d&to=now)
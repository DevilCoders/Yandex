[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=hanging+operations), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-hanging-operations)

## Описание
Мониторинг долго исполняемых операций yc-compute-tasks

## Подробности
Загорается при наличии исполняющихся более часа операциях. Скорее всего, в них что-то не так

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?viewPanel=4&orgId=1&from=now-3h&to=now)
- Проверить свежие операции на голове `sudo yc-compute-tasksctl operations list-processing -d 1h` на предмет ошибок
- Читать логи по операциям с ошибками `sudo journalctl OPERATION_ID=<id>`

## Ссылки
- [Дашборд в Графане](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1&from=now-3h&to=now)
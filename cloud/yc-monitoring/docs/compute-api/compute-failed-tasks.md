[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+failed+op), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-failed-operations)

## Описание
Мониторинг частоты падающих тасков yc-compute-tasks

## Подробности
Загорается при массовом падении и попытках ретрая тасков. Если таска завершается некорректно, то после исчерпания внутренних ретраев - она отправляется в вечный луп ретраев. Если данное явление массовое - увидим всплеск на графиках.

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?viewPanel=11&orgId=1&from=now-7d&to=now)
- Проверить свежие операции на голове `sudo yc-compute-tasksctl operations list-processing` на предмет ошибок
- Читать логи по операциям с ошибками `sudo journalctl OPERATION_ID=<id>`

## Ссылки
- [Дашборд в Графане](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1&from=now-7d&to=now)
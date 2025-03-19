[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-health-ydb%26namespace%3Dycloud)

## compute-health-ydb
Проверяет, что у сервиса `compute-health` всё хорошо с БД.

## Подробности
Периодически пингуется YDB, если возникают ошибки - загорается алерт.

## Диагностика
- Зайти на проблемный хост. Проверить состояние, логи сервиса:
    - `sudo systemctl status yc-compute-health`
    - `sudo journalctl -u yc-compute-health -f`

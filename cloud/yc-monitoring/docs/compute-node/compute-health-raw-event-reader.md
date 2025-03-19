[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-health-raw-event-reader%26namespace%3Dycloud)

## compute-health-raw-event-reader
Проверяет, что у сервиса `compute-health` работает чтение сырых событий из LB.

## Подробности

Сервис чтения сырых событий в `compute-health` отвечает за чтение событий из LogBroker и перекладывание их в БД.
Из всех нод `compute-health` чтение сырых событий работает только на одной.


## Диагностика
- Зайти на проблемный хост. Проверить состояние, логи сервиса:
    - `sudo systemctl status yc-compute-health`
    - `sudo journalctl -u yc-compute-health -f`

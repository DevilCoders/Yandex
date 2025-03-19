[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-health-calculator%26namespace%3Dycloud)

## compute-health-calculator
Проверяет, что у сервиса `compute-health` работает Calculator

## Подробности
Calculator в `compute-health` занимается вычислением значений агрегатов.
Ошибки в его работе скорее всего означают проблему в коде, или проблему с БД.

Статус `WARN` означает, что Calculator не включен, это указывает на проблемы в работе координатора. 

## Диагностика
- Зайти на проблемный хост. Проверить состояние, логи сервиса:
    - `sudo systemctl status yc-compute-health`
    - `sudo journalctl -u yc-compute-health -f`

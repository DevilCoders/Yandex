[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dscheduler-has-enough-nodes%26namespace%3Dycloud)

## scheduler-has-enough-nodes
Проверяет, что у сервиса `yc-scheduler` живо достаточное кол-во нод.

## Подробности
Если у Scheduler-а недостаточно живых нод, то это потенциальная проблема. Надо смотреть, что с нодами сервиса.
Мониторинг срабатывает если более 40% нод не в строю.

## Диагностика
- Зайти на каждую из голов. Проверить состояние, логи сервиса:
    - `sudo systemctl status yc-scheduler`
    - `sudo journalctl -u yc-scheduler -f`

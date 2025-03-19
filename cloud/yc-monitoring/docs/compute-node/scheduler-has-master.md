[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dscheduler-has-master%26namespace%3Dycloud)

## scheduler-has-master
Проверяет, что у сервиса `yc-scheduler` есть хотя бы один мастер.

## Подробности
Отсутствие рабочего мастера является проблемой, так как запросы на аллокации никто не обрабатывает.
В событиях есть описание в каком состоянии находится сервис на конкретной машине, в нем могут встречаться следующие статусы сервиса:
 - BrokenMaster
 - Unknown
 - Slave
 - Master
Мониторинг срабатывает только если нет ни одного ОК от сырых событий, при этом no data не учитывается

## Диагностика
- Зайти на одну из голов. Проверить состояние, логи сервиса:
    - `sudo systemctl status yc-scheduler`
    - `sudo journalctl -u yc-scheduler -f`

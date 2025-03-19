[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dscheduler-heartbeat%26namespace%3Dycloud)

## scheduler-heartbeat
Проверяет, что у сервиса `yc-scheduler` есть коннект до базы YDB и обновляет время последней успешной попытки.

## Подробности
Срабатывает если все события в CRIT

## Диагностика
- Зайти на одну из голов. Проверить состояние, логи сервиса:
    - `sudo systemctl status yc-scheduler`
    - `sudo journalctl -u yc-scheduler -f`
    - если в логах есть ошибка вида `ydb: operation error: NOT_FOUND` - перезапустить сервис `systemctl restart yc-scheduler`, тикет на проблему: [CLOUD-95212](https://st.yandex-team.ru/CLOUD-95212)

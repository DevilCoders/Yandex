[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dscheduler%26namespace%3Dycloud)

## scheduler
Проверяет, что сервис `yc-scheduler` работает (отвечает на healthcheck-ручку ##http://[::]:7000/v1/status##).

## Подробности
Сервис отказоустойчивый, запущен в нескольких экземплярах. Запросы обрабатывает один - мастер. Если упал один процесс из множества, это скорее всего не страшно (влияния на пользователей нет).

## Диагностика
- Зайти на голову. Проверить состояние, логи сервиса:
  - `sudo systemctl status yc-scheduler`
  - `sudo journalctl -u yc-scheduler -f`

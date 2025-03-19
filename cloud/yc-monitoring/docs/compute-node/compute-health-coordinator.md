[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-health-coordinator%26namespace%3Dycloud)

## compute-health-coordinator
Проверяет, что у сервиса `compute-health` работает координатор

## Подробности
Координатор в `compute-health` отвечает за принятие решений о том, чем занимаются ноды, путём регистрации нод в БД, и выбором роли для каждой из нод.  
Ошибки в его работе сигнализируют о проблемах в сервисе.

## Диагностика
- Зайти на проблемный хост. Проверить состояние, логи сервиса:
    - `sudo systemctl status yc-compute-health`
    - `sudo journalctl -u yc-compute-health -f`

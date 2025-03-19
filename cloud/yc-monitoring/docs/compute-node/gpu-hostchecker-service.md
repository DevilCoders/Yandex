[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dgpu-hostchecker-service)

## gpu-hostchecker-service
Проверка предназначена для мониторинга результатов запуска systemd сервиса
`yc-gpu-hostchecker`.

## Подробности
Указание сервиса `yc-gpu-hostchecker` в зависимостях к `compute-node`
приводит к тому, что при остановке сервиса `yc-gpu-hostchecker`
останавливается и `compute-node`. Чтобы избежать этого поведения
зависимости убраны и используется отдельный мониторинг конкретно
на сам systemd сервис. Отличие от проверки `gpu-hostchecker`
заключается в том, что `gpu-hostchecker` запускает ограниченный
набор проверок при каждом вызове скрипта мониторинга.
`gpu-hostchecker-service` проверяет systemd сервис, который
производит полную проверку железа на старте сервера.

## Диагностика
Для того, чтобы посмотреть причину ошибки запуска сервиса следует
выполнить команду `sudo journalctl -u yc-gpu-hostchecker` на машине
с алертом.

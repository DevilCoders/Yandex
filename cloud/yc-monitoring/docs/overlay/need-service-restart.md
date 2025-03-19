[Алерт need-service-restart в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dneed-service-restart)

## Что проверяет

Незвонящий, горит только жёлтым. Помогает при релизах не забыть перезапустить сервис.

Проверяет, накатывалась ли новая версия конфигов или пакетов после последнего запуска соответствующего сервиса.

## Если загорелось

- рестартуем сервис, **если уверены, что это нужно сделать:** `safe-restart <service_name>`

- если рестарт не требуется, то можно погасить проверку: `sudo /home/monitor/agents/modules/need-service-restart --reset <service_name>` либо `sudo rm /var/run/yc-need-service-restart/<service_name>`
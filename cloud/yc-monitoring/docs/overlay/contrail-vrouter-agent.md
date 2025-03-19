[Алерт contrail-vrouter-agent в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-agent)

## Что проверяет

Живость `contrail-vrouter-agent` на компьют-ноде. Проверяется через интроспекцию.

## Если загорелось

- если агент умер, то надо посмотреть, что с ним случилось, и запустить

- `systemctl status contrail-vrouter-agent`

- логи в `/var/log/contrail/contrail-vrouter-agent.log`

- `safe-restart --force contrail-vrouter-agent` — рестарт
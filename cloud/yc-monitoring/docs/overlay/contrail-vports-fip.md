[Алерт contrail-vports-fip в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vports-fip)

## Что проверяет

Что FIP-ы на портах по данным Compute Node (= в state-файлах на диске) и по данным `contrail-vrouter-agent` (= в интроспекции) совпадают. Мониторинг изначально делался для [CLOUD-38580](https://st.yandex-team.ru/CLOUD-38580).

## Если загорелось

- может загореться при рестарте `contail-control`

- «отклеивание» FIP означает потерю внешней связности у инстанса. Посмотреть историю проблемы на конкретной компьют-ноде можно с помощью команды:

- `sudo yc-contrail-tool logs vrouter find-lost-ips -f /var/log/contrail/contrail-vrouter-agent.log`

- в большинстве случаев помогает рестарт `safe-restart --force contrail-vrouter-agent`

- если нет, возможно это [CLOUD-58797](https://st.yandex-team.ru/CLOUD-58797), попробовать по инструкции оттуда
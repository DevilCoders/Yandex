[Алерт vrouter-dropstats-l3-route-errors в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvrouter-dropstats-l3-route-errors)

[Алерт vrouter_dropstats-l2-l3-minor-route_errors в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvrouter_dropstats-l2-l3-minor-route_errors)

## Что проверяет

Обычно связано с пакетом, который мы не можем смаршрутизировать на виртуалке. При массовом загорани — что-то отклеилось в vrouter/control

(кроме случаев `ds_flow_action_drop`, когда это Short Flow с другим reason, или вообще из-за SG)

## Если загорелось

- перейти на дашборд по ссылке из алерта

- посмотреть, у какой виртуалки растут if_ds, подампить интерфейс или половить пакеты через `yc-bpf-trace skb trace -P drop`

- [почитать маршрутые логи агента](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/Contrail-VRouter-Agent-Route-Logs-XMPP-Messages/)

- [как интерпретировать dropstats & short flows](https://wiki.yandex-team.ru/cloud/devel/sdn/dropstats/)
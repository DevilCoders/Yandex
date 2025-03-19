[Алерт yc-network-flow-collector-stuck в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dyc-network-flow-collector-stuck)

Проверяет значение задержки обрабатываемых на компьют ноде flow логов.
В случае если загорелась проверка, необходимо посмотреть логи yc-network-flow-collector на диске - /var/log/yc/network-flow-collector/network-flow-collector.log

Чаще всего проблемы связаны с невозможностью похода в балансер logbroker'а
На это дело есть авторекавери, однако если проверка горит продолжительное время нужно перезапустить сервис вручную, а так же спросить о состоянии облачного логброкера у /duty logbroker

В случае если и это не помогло, призовите на помощь alg37.

[Вики](https://wiki.yandex-team.ru/cloud/devel/sdn/vpc-accounting/overview/)
# alive_targets_perc

**Значение:** В случае WARN hc-node получает timeout более чем на 75% чеков, в случае CRIT более чем на 90%
**Воздействие:** Не проходят healthcheck'и и все реалы потенциально могут быть выключены. Пободная проблема вероятно сигнализирует о проблемах связности.
**Что делать:** Возможные причины:
1. Развалились lb-node - смотреть, какие мониторинги горят на lb-node. Посмотреть список правил в vpp:
`sudo vppctl sh yclb rules`
Можно посмотреть графики (NB: везде суммы по всем lb-node) количества vip'ов ([prod](https://solomon.cloud.yandex-team.ru/?cluster=cloud_prod_ylb&project=yandexcloud&service=loadbalancer_node&env=prod&host=loadbalancer-node-myt3&graph=yandexcloud-ylb-prod-dataplane-lb-node-vips) [preprod](https://solomon.cloud.yandex-team.ru/?cluster=cloud_preprod_ylb&project=yandexcloud&service=loadbalancer_node&env=preprod&host=loadbalancer-node-myt1&graph=yandexcloud-ylb-preprod-dataplane-lb-node-vips)), общего количества правил ([prod](https://solomon.cloud.yandex-team.ru/?cluster=cloud_prod_ylb&project=yandexcloud&service=loadbalancer_node&env=prod&host=loadbalancer-node-myt3&graph=yandexcloud-ylb-prod-dataplane-lb-node-rules) [preprod](https://solomon.cloud.yandex-team.ru/?cluster=cloud_preprod_ylb&project=yandexcloud&service=loadbalancer_node&env=preprod&host=loadbalancer-node-myt1&graph=yandexcloud-ylb-preprod-dataplane-lb-node-rules)). Если там не хватает правил, то смотреть в проверку lb-node-ping и логи lb-node. Если выключение пришло от lb-ctrl или правила не пришли совсем, смотрим в его мониторинги.
`sudo journalctllb -u yc-loadbalancer-node -S -10m`
В любом случае, тут может помочь рестарт vpp (если это не единственный живой vpp в зоне):
`sudo systemctl restart vpp`
2. Развалились hc-node - смотреть в логи hc-node:
`sudo journalctllb -u yc-healthcheck-node -S -5m`
Смотреть в графики handle_fail на hc-node [prod](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=healthcheck_node&l.host=hc-node-rc1*&l.name=check_timeouts&graph=auto&transform=differentiate) [preprod](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_preprod_ylb&service=healthcheck_node&l.host=hc-node-rc1*&l.name=check_timeouts&graph=auto&transform=differentiate). Если fail rate на одной из нод больше, чем на других, выключить её:
`sudo systemctl stop yc-healthcheck-node`
3. Развалились hc-ctrl - смотреть мониторинги на хостах, смотреть в отгружаемые правила:
`pssh run -p3 'curl -s 0x0:4050/debug/targets' C@cloud_prod_ylb_stable_hc-ctrl`
смотреть в логи
`sudo journalctllb -u yc-healthcheck-ctrl -S -10m`
4. Развалилась сеть между hc-node. Дебажим сеть с гипервизоров hc-node и lb-node, смотрим, вылетают ли пакеты между ними и между lb-node и реалами (можно призвать /duty vpc).

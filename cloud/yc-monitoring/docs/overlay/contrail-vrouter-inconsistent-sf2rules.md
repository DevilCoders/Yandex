[Алерт contrail-vrouter-inconsistent-sf2rules в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-inconsistent-sf2rules)

## Что проверяет

Корректность вычисления дерева правил super-flow-v2+, сравнивая с симулятором.

## Если загорелось

1. Посмотреть в графики инстанса и compute-node. Если нет корреллирующего роста дропов
(особенно `Flow Unusable`) и short flows `Short flow vrouter install failed`,
`Short flow Audit Entry`, вероятно импакта на датаплейн нет.
   - Иначе добавить флаги `super-flow-v2.1` и `super-flow-v2.2` в блеклист
   через add-to-black-list аналогично [инструкции по Super-Flow-v2+](https://wiki.yandex-team.ru/users/sklyaus/super-flows-shop/#superflowsv21)

2. Посмотреть, какие ноды не совпали в логах yc-contrail-monitor-vrouter:
```
sudo journalctl -u yc-contrail-monitor-vrouter -S -15m | grep  tap37l7d1p32-0
```

Найти релевантный проблеме тикет в эпике https://st.yandex-team.ru/CLOUD-42388 или
завести новый.

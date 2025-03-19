# cgw-stats-balancer-rules
[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?project=&query=service%3Dcgw-stats-balancer-rules)

## Проверяет
Соответствие балансерных enabled правил, выставленных сервисом ```lb-node``` в ```vpp``` и всех (в upstream/downstream) анонсов соответсвующих правил в ```gobgp```.

## Что делать
В логах сервиса ```yc-cgw-stats``` посмотреть vip'ы, по которым загорелась проверка:
```"2022-03-24T12:28:24.405Z","caller":"handlers/comparator.go:232","msg":"unset bgp announce for the vip","vip":"x.x.x.x"}```

* Проверить, что этот vip есть(и enabled) или нет в ```vpp``` - ```sh yclb rules```
* Проверить, что этот vip есть в ```gobgp``` - ```gobgp global rib -a <family>```

В случае несоответствия состояния - прикопать в дежурный тикет всю информацию выше. Рекавери - ```sudo yavpp-restart```
Если же все маршруты выставлены верно и мониторинг false-positive - приходи в чатик.

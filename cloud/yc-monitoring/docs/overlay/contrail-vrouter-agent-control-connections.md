[Алерт contrail-vrouter-agent-control-connections в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-agent-control-connections)

## Что проверяет

Что contrail-vrouter-agent имеет два стабильных подключения к control-ам.

Загорается в случаях:

- нет одного контрола (WARN)

- нет обоих контролов (CRIT) - *скорее всего есть импакт на клиентов, см. e2e*

- недавно пропадали оба контрола (CRIT) - *скорее всего есть импакт на клиентов, см. e2e*

## Если загорелось

Проверить живость contrail-control-ов, загрузку control-ов по CPU, рестарты contrail-control, связность IPv4 Underlay.
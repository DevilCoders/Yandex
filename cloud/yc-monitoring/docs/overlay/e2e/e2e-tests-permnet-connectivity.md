[Алерт e2e-tests-permnet-connectivity в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-tests-permnet-connectivity)

Тесты запускаются на гипервизорах. На каждом гипервизоре запускается одна виртуалка и никогда не удаляется автоматически, в нее эти тесты и ходят. Тесты запускаются на каждой ## compute-node** раз в **120сек**

- **test_vm_oldest** — проверяет что постоянная виртуалка создана не слишком давно (не больше часа назад). Если регулярно загорается, то это обозначает что кто-то удаляет эти постоянные виртуалки или они сами удаляются по какой-то причине.

- **test_ipv6_connectivity_for_permvm** — заходим на виртуалку, проверяем, что с нее пингуется 2a02:6b8::1(ns1.yandex.ru).

- **test_ipv4_connectivity** — в сети highnet включен nat-gateway. Проверка проверяет v4 связность через NAT так же как ipv4_connectivity в [e2e-tests-network-fip-тесте](e2e-tests-network-fip.html).
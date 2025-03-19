[Алерт e2e-tests-dc-permnet-connectivity в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-tests-dc-permnet-connectivity)

- **test_direct_connect_connectivity**

- Проверяют связность в cloud interconnect сценарии. Запускается на каждом cgw-dc, и проверяет связность между 2-мя vpc, соединёнными через cloud-interconnect через физическую перемычку на оконечном бордере.

В рамках одного запуска проверяются два типа подключения относительно cgw:

- **[local]** — обе сети подключены только через этот cgw, весь трафик ходит только через него.

- **[az]** — prod-like подключение, трафик в обе стороны ходит через любой из cgw в ecmp группе, при этом тесты запускаются без kikimr-lock'а

Также проверяются два сценария.

- (без суффикса), ping-mesh, 2 виртуалки пингуют друг друга.

- **_wellknown**, каждая виртуалка пингует 3 адреса из 192.168.255.0/30 (.1 — адрес одного из интерфейсов в перемычке на M9, .2 — аналогично на STD, .3 — anycast адрес, установленный на каждом из iBR) более подробное описание схемы лупбеков можно найти на [вики](https://wiki.yandex-team.ru/cloud/devel/NetInfra/CloudInterConnect/cic_e2e/#adresalupbekov)
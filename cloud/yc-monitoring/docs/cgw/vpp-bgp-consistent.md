# vpp-bgp-consistent

**Значение:** Неконсистентное состояние vpp и gobgp для IPv4 VIP'ов.
**Воздействие:** В gobgp есть анонсы IPv4 vip'ов, которых нет в vpp - по таким vip'ам проливаем трафик.
**Что делать:** Ошибки вида: "Extra announces in gobgp, vips: set([IPv4Address(u'1.1.1.1')])"
VIP 1.1.1.1 анонсируется на gobgp, но его нет в vpp. Нужно:
1. Проверить, жива ли lb-node: `sudo systemctl status yc-loadbalancer-node` . Если не жива, стартануть: `sudo systemctl start yc-loadbalancer-node`.
2. Посмотреть, что делает lb-node:
* `curl -s 0x0:4050/ping` - если тут `state.handled` с непустым error - вероятно, lb-node получает начальные правила от lb-ctrl (либо кто-то из них завис). В этом случае см. действия по lb-node-ping, но также есть смысл какое-то посмотреть в логи lb-node и lb-ctrl
* `sudo journalctllb -fu yc-loadbalancer-node` - если там много сообщений типа "vpper: insert", "vpper: enable", "vppctl: next" - то lb-node прокачивает правила от lb-ctrl'а.
* посмотреть в потребление cpu процессами yc-loadbalancer-node, gobgp, go2vpp (последний - `python -m yavpp.gobgp`). Если они жрут 100+% cpu каждый, то они молотят начальное состояние. Это нормально, если сервисы стартовали недавно, что можно посмотреть в `sudo systemctl status go2vpp` и `sudo systemctl status gobgp`.
3. Если предыдущие пункты не дали результата, то семь бед - один `sudo systemctl restart vpp`. ВАЖНО: убедись, что это не единственный оставшийся в живых в зоне vpp.

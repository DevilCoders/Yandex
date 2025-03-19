[Алерт e2e-security-groups-new в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-security-groups-new)

[Алерт e2e-security-groups-perm в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-security-groups-perm)

[Алерт e2e-security-groups-crossaz в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3De2e-security-groups-crossaz)

Бывает `e2e-security-groups-new`, `e2e-security-groups-perm` и `e2e-security-groups-crossaz`.

Запускаются с **oct-голов**, создают сеть, секьюрити-группы и три инстанса. Проверяют, что секьюрити-группы корректно ограничивают хождение трафика между этими инстансами.

- `-new` создаёт все сущности (сети, подсети, инстансы, секьюрити-группы) с нуля при каждом запуске, а в конце (или в начале следующего запуска) удаляет.

- `-perm` создаёт сущности один раз, а при запуске просто проверяет, что трафик всё ещё ходит или не ходит согласно правилами.

- `-crossaz` запускается на одной голове во всём стенде, создаёт все сущности с нуля при каждом запуске, причём инстансы и подсети размещает в разных AZ. Запускается только на стендах, в которых больше одной AZ.

Последовательность действий проще всего проследить в коде: метод `SecurityGroupTestContext.test()` в [test_security_groups.py#208](https://bb.yandex-team.ru/projects/CLOUD/repos/e2e-tests/browse/e2e_tests/yc_e2e_tests/test_security_groups.py#208). Проверяются TCP и UDP соединение, секьюрити-групп всего две: исходящего трафика и для входящего. Конкретные порты указаны в правилах в начале исходника (константы `INGRESS_TCP_2000_4000` и около того). На `vm1` навешивается первая, на `vm2` вторая, на `vm3` — никакая.

Прохождение трафика между инстансами проверяются с помощью запуска команд `nc -l -p <port>` на сервере и `echo text | nc <address> <port>` на клиенте.

Если тест упал, то он постарается подробно в сообщении описать, какой трафик и куда не дошёл. В логах (`/var/log/e2e/e2e-security-groups-*`) будут подробные сообщения о созданных сетях и инстансах, а также обо всех запущенных проверках.
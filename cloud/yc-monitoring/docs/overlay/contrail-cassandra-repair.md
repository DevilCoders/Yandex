[Алерт contrail-cassandra-repair в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-cassandra-repair)

## Что проверяет

Успешность прохождения [anti-entropy repair](https://docs.datastax.com/en/cassandra/3.0/cassandra/operations/opsRepairNodesWhen.html). См. `sudo crontab -l | grep repair`

Repair на кластере запускается на каждой ноде раз в сутки. Достаточно прохождения одного repair в течение суток. Если repair не проходят, через некоторое время данные на нодах кассандры начнут отличаться друг от друга. Возможно воскрешение «удаленных» записей.

 

Мониторинг проверяет лог cron-джобы на сообщение об успешно проведённой операции.

На **свежей машине** будет гореть WARN-ом c `No log file: repair hasn't run yet` не более одного дня до первого запуска, это нормально.

## Полезные ссылки

- [https://wiki.yandex-team.ru/cloud/devel/sdn/duty/alert-contrail-cassandra-repair/](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/alert-contrail-cassandra-repair/)

- [https://st.yandex-team.ru/CLOUD-3602](https://st.yandex-team.ru/CLOUD-3602)

- [https://st.yandex-team.ru/CLOUD-8330](https://st.yandex-team.ru/CLOUD-8330)

- ~~https://wiki.yandex-team.ru/cloud/devel/sdn/opencontrail-cassandra/periodic-maintainance/~~ — местами **значительно** устарело

## Если загорелось

- обычно ждет утра, **но надо починить за gc_grace_seconds (сутки)**

- можно временно выключить cron-джобу на всех нодах, чтобы разобраться

- можно рестартовать ноды кассандры одна за другой, чтобы остановить зависшую операцию repair

- **никогда не запускай** `/usr/bin/contrail-cassandra-repair` одновременно больше, чем на одной ноде.
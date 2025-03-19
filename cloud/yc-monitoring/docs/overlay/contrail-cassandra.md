[Алерт contrail-cassandra в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-cassandra)

## Что проверяет

Живость кассандры: подключение к портам thrift:9160 и cql:9042. Кассандра работает в кластере из пяти нод в каждой AZ. Без кассандры не работает Contrail API. Выпадение одной или двух нод не прерывает работу кластера. Выпавшую из кластера ноду надо вернуть в течение суток обратно. В противном случае автоматика [не даст сервису запуститься](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/howto-start-cassandra-if-check-forbids-join-cluster/).

## Если загорелось

- `systemctl status cassandra`, `journalctl -u cassandra -n 13` 

- `nodetool status` — внутренняя диагностика кластера кассандры

- логи в `/var/log/cassandra/system.log`

- выполнить запрос к базе: `cqlsh $(awk '/^listen_address/ {print $2}' /etc/cassandra/cassandra.yaml) -e "SELECT * FROM config_db_uuid.obj_uuid_table LIMIT 1"`
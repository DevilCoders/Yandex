[Алерт contrail-dns в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-dns)

## Что проверяет

Сложная проверка, состоящая из нескольких частей. Проверяет корректность и внутреннее состояние сервиса `contrail-dns`.

**Практически неактуальна на всех стендах**, так как переехали на Cloud DNS.

Выглядит примерно так: `contrail-dns: OK (started 10 days ago), dns introspection: OK, mdb rpz: OK (mdb.yandexcloud.net), dns+named data filling: OK ([ifmap -> dns | vDNSes added: 0, records added: 14] [dns -> named | sent: 22 (avg. 9.27/sec), refused: 0/662 tries])`

Состоит из следующих частей:

- `contrail-dns` — наличие процесса `contrail-dns`.

- `dns introspection` — ответ сервиса `contrail-dns` на порту интроспекции (если сервису очень плохо, то может не ответить).

- `mdb rpz` — наличие RPZ-зоны для mdb.yandexcloud.net (или аналогичной зоны в других кластерах). Может загораться жёлтым, если поймали момент перезаписи конфига. На этот WARN ничего не делаем.

- `dns+named data filling` — накачка данными по цепочке `ifmap` → `contrail-dns` → `contrail-named`. Если данные ещё в пути, то конкретный инстанс `contrail-named` может отвечать неправильно. Диагностирует через интроспекцию `contrail-dns`.

- `log` — наличие известных плохих ошибок в логе

- `log format` — корректность лог. Лог пишется в JSON-формате, по одному событию в строке.

## Если загорелось

- позовите кого:andgein :-)

- `systemctl status contrail-dns`

- логи в `/var/log/contrail/contrail-dns.log`

- `sudo journalctl -u contrail-dns-reload -f`

- рестартить через `safe-restart --force contrail-dns`. Это рестартанёт и `contrail-dns`, и `contrail-named`, !!что приведёт к недоступности DNS на этой ноде в течение 10+ минут!. Поэтому на следующей голове можно делать только через 20-30 минут. Если знаешь, что делаешь, можно рестартовать только `systemctl restart contrail-dns`. В этом случае `contrail-named` продолжит работать и обрабатывать запросы пользователей.
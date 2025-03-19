## MDB search sender

It sends `search_queue.docs` from metadb to LogBroker - https://st.yandex-team.ru/MDB-4409.

`mdb-search-producer` is:
- `internal/metadb` producer API to `dbaas_metadb`
- `internal/producer` producer logic
- `internal/monitoring` monrun scripts

Producer contains of two goroutine
1. Send new documents to LogBroker
2. Receive LogBroker responses

Send goroutine looks like

[image source: https://a.yandex-team.ru/arc/trunk/arcadia/junk/wizard/tex/mdb-search-producer/producer.tex](https://jing.yandex-team.ru/files/wizard/producer.png "producer schema")


### Local

Fill config.

```yaml
log_level: Debug
metadb:  # Your metadb config. I use local
  addrs: ["localhost:5432"]
  db: "dbaas_metadb"
  user: "wizard"
  sslmode: "disable"
logbroker: # Your LB config
  endpoint: vla.logbroker.yandex.net
  topic: "/logbroker-playground/wizard/demo-topic"
  oauth:
    token: "..."
```

Make && run

    ya make
    ./cmd/mdb-search-producer/mdb-search-producer --genconfig
    # fill config
    ./cmd/mdb-search-producer/mdb-search-producer

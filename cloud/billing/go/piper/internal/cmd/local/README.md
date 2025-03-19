# Локальный запуск

- установить docker и docker-compose, если нет
- собрать docker-образ `registry.yandex.net/dev-local-lbk:stable`:

```shell
cd ${ARCADIA_ROOT}/cloud/billing/go/dev/logbroker
./build.sh
```

- запустить логброкер (и ydb вместе с ним):

```shell
cd ${ARCADIA_ROOT}/cloud/billing/go/piper/internal/cmd/local
docker-compose up lbk -d
```

- дождавшись запуска логброкера и ydb, запустить `ydb-client` для накатки миграций на базу, и `unified-agent`

```shell
docker-compose up ydb-client ua -d
```

- адаптировать `config.yaml`, под желаемую ситуацию:
    - для запуска решардера отключить `resharder.disabled`
    - для запуска презентера отключить `dump.source.presenter.disabled`
- собрать и запустить приложение:

```shell
ya make
./local -c config.yaml
```

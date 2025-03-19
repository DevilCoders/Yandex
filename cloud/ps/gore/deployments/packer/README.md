Здесь лежат packer-конфигурации для образов, используемых в команде GoRe. Список образов:

- `base` — базовый образ, собранный из Ubuntu 20.04 LTS. Поддерживает один сетевой интерфейс (IPv4 + IPv6), идёт с установленными ssh, cauth, hbf и osquery. За софтом ходит в облачный и яндексовый репозитории.

# Сборка образов

Сборка образов по умолчанию происходит в фолдере [`gore`](https://console.cloud.yandex.ru/folders/yc.gore.service-folder), в нём есть все необходимые сети и дырки.

Для сборки образа нужно почти всегда определить следующие переменные окружения:

```
export FOLDER_ID=yc.gore.service-folder
export SUBNET_ID=e9btq8g1hia7hpbcvibc                  # Подсеть для создания инстанса. С доступом к IPv6-сети Яндекса и к облачному дисту. В препроде — [bucpu8k7jbh8kb7h5pb9](https://console-preprod.cloud.yandex.ru/folders/yc.gore.service-folder/vpc/network/c64ndm3al3cdchq1n7t4).
export ZONE_ID=ru-central1-a                           # Зона, в которой будет создан инстанс.
export YC_API_ENDPOINT=api.cloud.yandex.net:443        # Для препрода — api.cloud-preprod.yandex.net:443
export YC_OAUTH_TOKEN="<здесь токен>"
```

Токен `YC_OAUTH_TOKEN` можно получить здесь: https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb

По умолчанию готовый образ окажется в указанном фолдере (в случае `gore` — [здесь](https://console.cloud.yandex.ru/folders/yc.gore.service-folder/compute?section=images)).

Для запуска, собственно, сборки образа, запустите packer:

```
packer build ./packer.json
```

**Важная особенность:** чтобы packer мог сходить на создаваемый инстанс, доступ на который есть только через `pssh`, надо либо настроить на ноутбуке `pssh-agent`, 
либо запускать сборку образа на «сборочном хосте» (`pssh -A deploy.cloud.yandex.net`), оттуда есть доступ напрямую.


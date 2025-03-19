Gore bot - https://wiki.yandex-team.ru/cloud/devel/gore
Создать сервис - `curl -v -H "Authorization: OAuth $GORE_OAUTH" -XPOST -d "@ycsearch.json" "https://resps-api.cloud.yandex.net/api/v0/services"`
Обновить сервис - `curl -kv -H "Authorization: OAuth $GORE_OAUTH" -XPATCH -d "@ycsearch.json.patch" "https://resps-api.cloud.yandex.net/api/v0/services/search"`

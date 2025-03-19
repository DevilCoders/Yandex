
#### MDB API requests:

Запросы в MDB API, действия над ресурсами MDB (кластер, фолдер, пользователь, хост)

#### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/mdb/api_request) / [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/mdb/api_request)

|column| description |
|--|--|
| request_id | уникальный идентификатор запроса |
| user_id | идентификатор пользователя |
| user_type| тип пользователя (сервисный аккаунт или пользовательский аккаунт) |
| cloud_id | идентификатор облака |
| folder_id | идентификатор каталога |
| cluster_id | идентификатор кластера (не заполняется для python api) |
| iso_eventtime | дата-время события |
| endpoint | вызванный метод |

## IAM Clouds
#iam #clouds

Таблица [Облаков](https://cloud.yandex.ru/docs/resource-manager/concepts/resources-hierarchy#cloud).

История изменений: [clouds_history](../clouds_history)


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                     | Источники                                                                                                                                              |
|---------|---------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [Clouds](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/clouds)      | [raw_Clouds](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/hardware/default/identity/r3/clouds)       |
| PREPROD | [Clouds](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/clouds)   | [raw_Clouds](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/hardware/hardware/default/identity/r3/clouds) |



### Структура
| Поле                              | Описание                                                                  |
| --------------------------------- |-------------------------------------------------------------------------- |
| cloud_id                          | Идентификатор облака.                                                     |
| cloud_name                        | Наименование облака.                                                      |
| organization_id                   | Идентификатор [организации](../organizations).                            |
| status                            | Статус облака (creating, active, blocked).                                |
| created_by_iam_uid                | Пользователь, создавший облако.                                           |
| modified_at                       | Дата и время модификации.                                                 |
| created_at                        | Дата и время создания.                                                    |
| deleted_at                        | Дата и время удаления.                                                    |
| default_zone                      | Зона развертывания ресурсов, создаваемых в облаке, по умолчанию.          |
| permission_stages                 | Список разрешительных флагов, проставленных для Облака.                   |
| permission_mdb_sql_server_cluster | Признак, проставлен ли флаг `MDB_SQLSERVER_CLUSTER` в `permission_stages`.|

### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.

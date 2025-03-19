## IAM Clouds History
#iam #clouds #history

Историческая таблица изменений облака в IAM.

Актуальная таблица: [clouds](../clouds).

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                     | Источники                                                                                                                                              |
|---------|---------------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [Clouds_History](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/clouds_history)      | [raw_Clouds_History](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/hardware/default/identity/r3/clouds_history)      |
| PREPROD | [Clouds_History](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/clouds_history)   | [raw_Clouds_History](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/hardware/hardware/default/identity/r3/clouds_history) |



### Структура
| Поле                              | Описание                                                                      |
| --------------------------------- | ----------------------------------------------------------------------------- |
| cloud_id                          | идентификатор облака                                                          |
| cloud_name                        | имя облака                                                                    |
| organization_id                   | идентификатор организации                                                     |
| status                            | статус облака (creating, active, blocked)                                     |
| created_by_iam_uid                | пользователь, создавший облако                                                |
| modified_at                       | дата и время модификации                                                      |
| created_at                        | дата и время создания                                                         |
| deleted_at                        | дата и время удаления                                                         |
| default_zone                      | зона развертывания ресурсов, создаваемых в облаке, по умолчанию.              |
| permission_stages                 | Список разрешительных флагов, проставленных для Облака.                       |
| permission_mdb_sql_server_cluster | Признак, проставлен ли флаг `MDB_SQLSERVER_CLUSTER` в `permission_stages`.    |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.

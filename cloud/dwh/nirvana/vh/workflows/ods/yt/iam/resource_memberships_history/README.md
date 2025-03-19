## Resource memeberships history

Историческая таблица членства пользователей в ресурсе.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                                                   | Источники                                                                                                                                                      |
| --------- |---------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD      | [resource_memberships_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/resource_memberships_history) | [resource_memberships_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/iam/resource_memberships_history)    |
| PREPROD   | [resource_memberships_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/resource_memberships_history)       | [resource_memberships_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/iam/resource_memberships_history) |


### Структура
| Поле                | Описание                                                  |
|---------------------|-----------------------------------------------------------|
| iam_resource_id     | Идентификатор ресурса                                     |
| iam_resource_type   | Тип ресурса                                               |
| iam_user_id         | Идентификатор пользователя                                |
| modified_ts         | Время изменения в Timestamp                               |
| modified_dttm_local | Время изменения МСК                                       |
| deleted_ts          | Время удаления Timestamp                                  |
| deleted_dttm_local  | Время удаления МСК                                        |
| metadata_user_id    | ??? Идентификатор пользователя, исполнившего действие ??? |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.

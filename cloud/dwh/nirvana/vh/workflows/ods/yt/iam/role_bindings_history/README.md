## Role bindings history

Теги: ods, yt, iam, role_bindings_history

Историческая таблица привязок ролей.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                                      | Источники                                                                                                                                                                                           |
| --------- |--------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD      | [role_bindings_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/role_bindings_history) | [role_bindings_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/default/identity/r3/role_bindings_history)    |
| PREPROD   | [role_bindings_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/role_bindings_history)      | [role_bindings_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/hardware/default/identity/r3/role_bindings_history) |


### Структура
| Поле                      | Описание                                           |
|---------------------------|----------------------------------------------------|
| modified_ts               | Время модификации Timestamp                        |
| modified_dttm_local       | Время модификации МСК                              |
| iam_resource_id           | Идентификатор ресурса, на котором выдана роль      |
| role_slug                 | Роль                                               |
| iam_subject_id            | Субъект                                            |
| deleted_ts                | Время удаления в Timestamp                         |
| deleted_dttm_local        | Время удаления МСК                                 |
| iam_binding_id            | Идентификатор привязки роли                        |
| is_system                 | Роль привязана системой (сервисом из облака)       |
| iam_resource_cloud_id     | Идентификатор облака, на котором расположен ресурс |
| resource_type             | Тип ресурса                                        |
| subject_type              | Тип субъекта                                       |
| managed_by                | Кто управляет привязкой ролей                      |
| iam_top_level_resource_id | Идентификатор верхнеуровнего ресурса               |
| top_level_resource_type   | Тип верхнеуровневого ресурса                       |
| metadata_user_id          | Идентификатор пользователя                         |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.

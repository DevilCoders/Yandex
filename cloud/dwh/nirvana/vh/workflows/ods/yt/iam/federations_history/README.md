## IAM federations history

Историческая таблица федераций.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                                       | Источники                                                                                                             |
| --------- |---------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------|
| PROD      | [federations_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/federations_history) | [federations/saml_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/default/identity/r3/federations/saml_history) |
| PREPROD   | [federations_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/federations_history) | [federations/saml_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/default/identity/r3/federations/saml_history)  |


### Структура
| Поле                      | Описание                  |
|---------------------------|---------------------------|
| iam_federation_id         | Идентификатор федерации   |
| modified_ts               | Время изменения Timestamp |
| modified_dttm_local       | Время изменения МСК       |
| created_ts                | Время создания Timestamp  |
| created_dttm_local        | Время создания МСК        |
| deleted_ts                | Время удаления Timestamp  |
| deleted_dttm_local        | Время удаления МСК        |
| description               | Описание федерации        |
| iam_folder_id             | Идентификатор папки       |
| issuer                    |                           |
| name                      | Название федерации        |
| sso_binding               |                           |
| sso_url                   |                           |
| cookie_max_age            | Максимальное время Cookie |
| autocreate_users          |                           |
| encrypted_assertions      |                           |
| status                    | Статус федерайии          |
| case_insensitive_name_id  |                           |
| case_insensitive_name_ids |                           |
| set_sso_url_username      |                           |
| metadata_user_id          |                           |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.

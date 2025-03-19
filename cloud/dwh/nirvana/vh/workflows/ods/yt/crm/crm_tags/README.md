## crm_tags
#crm #crm_tags

Содержит информацию о тэгах.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                    | Источники                                                                                                                 |
|---------|--------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_tags](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_tags) | [raw-crm_tags](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_tags) |
| PREPROD | [crm_tags](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_tags) | [raw-crm_tags](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_tags) |


### Структура

| Поле                     | Описание                                                                                                                         |
|--------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| assigned_user_id         | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| created_by               | создано                                                                                                                          |
| date_entered_ts          | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local  | дата время ввода, local tz                                                                                                       |
| date_modified_ts         | дата время изменения, utc                                                                                                        |
| date_modified_dttm_local | дата время изменения, local tz                                                                                                   |
| deleted                  | был ли тэг удален                                                                                                                |
| crm_tag_description      | описание тэга                                                                                                                    |
| crm_tag_id               | id тэга , `PK`                                                                                                                   |
| modified_user_id         | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_tag_name             | название тэга                                                                                                                    |
| crm_tag_name_lower       | название тэга в нижнем регистре                                                                                                  |
| crm_source_id            | id источника                                                                                                                     |
| source_meta              |                                                                                                                                  |
| source_typ               | тип источника                                                                                                                    |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.

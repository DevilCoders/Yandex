## crm email address bean relations
#crm #crm_email_address_bean_relations

Содержит информацию о связях измерений с другими объектами в CRM.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                                    | Источники                                                                                                                                                        |
|---------|--------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_email_address_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_email_address_bean_relations) | [raw-crm_email_address_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_email_addr_bean_rel) |
| PREPROD | [crm_email_address_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_email_address_bean_relations) | [raw-crm_email_address_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_email_addr_bean_rel) |


### Структура

| Поле                     | Описание                                                                                                         |
|--------------------------|------------------------------------------------------------------------------------------------------------------|
| bean_id                  | id группы                                                                                                        |
| bean_module              | модуль                                                                                                           |
| date_created_ts          | дата создания, utc                                                                                               |
| date_created_dttm_local  | дата создания, local tz                                                                                          |
| date_modified_ts         | дата время модификации, utc                                                                                      |
| date_modified_dttm_local | дата время модификацииа, local tz                                                                                |
| deleted                  | была ли удалена                                                                                                  |
| crm_email_id             | id [email](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_email_addresses), `FK` |
| id                       | id                                                                                                               |
| primary_address          | основной адресс?                                                                                                 |
| reply_to_address         | ответать ли на адрес?                                                                                            |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.

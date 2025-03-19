## crm opportunities contacts
#crm #crm_opportunities_contacts

Связь контактов со сделками.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                                        | Источники                                                                                                                                                     |
|---------|--------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_opportunities_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_opportunities_contacts) | [raw-crm_opportunities_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_opportunities_contacts) |
| PREPROD | [crm_opportunities_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_opportunities_contacts) | [raw-crm_opportunities_contacts](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_opportunities_contacts) |


### Структура

| Поле                     | Описание                                                                                                        |
|--------------------------|-----------------------------------------------------------------------------------------------------------------|
| crm_contact_id           | id [контакта](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_contacts), `FK`    |
| crm_contact_role         | роль контакта в сделке                                                                                          |
| date_modified_ts         | дата время модификации, utc                                                                                     |
| date_modified_dttm_local | дата время модификацииа, local tz                                                                               |
| deleted                  | была ли удалена                                                                                                 |
| id                       | id                                                                                                              |
| crm_opportunity_id       | id [сделки](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunities), `FK` |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.

## Tag bean relations
#crm #tag_bean_relations

Таблица связывает тэг и объекты.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                        | Источники                                                                                                                                   |
|---------|----------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [tag_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/tag_bean_relations) | [raw-tag_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_tag_bean_rel) |
| PREPROD | [tag_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/tag_bean_relations) | [raw-tag_bean_relations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_tag_bean_rel) |


### Схема

| Поле                     | Описание                                                                                                   |
|--------------------------|------------------------------------------------------------------------------------------------------------|
| tag_bean_relation_id     | relation_id сущности, с которой связан тег                                                                 |
| crm_bean_id              | id объекта в CRM, с которым связан тэг                                                                     |
| crm_bean_module          | модуль объекта в CRM, с которым связан тэг                                                                 |
| date_modified_ts         | дата время модификации, utc                                                                                |
| date_modified_dttm_local | дата время модификацииа, local tz                                                                          |
| deleted                  | была ли связь удалена                                                                                      |
| crm_tag_id               | id [тэга в CRM](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_tags), `FK` |


### Загрузка
Статус загрузки: активна

Периодичность загрузки: 1h.

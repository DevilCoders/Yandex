## applications
'#backoffice #cpu-util #applications

Вычитывает последний (актуальный) снапшот таблицы `applications`. Заполненная регистрационная форма для участника мероприятия.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [PII структура](#PII-структура)
4. [Загрузка.](#загрузка)

### Расположение данных
| Контур      | Расположение данных                                                                                                              | Источники                                                                                                                           |
|-------------|----------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------|
| PROD        | [ODS_APPLICATIONS](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/backoffice/applications)        | [RAW_CPU_UTIL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/postgresql/backoffice/applications)    |
| PREPROD     | [ODS_APPLICATIONS](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/backoffice/applications)     | [RAW_CPU_UTIL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/postgresql/backoffice/applications) |
| PII_PROD    | [PII_APPLICATIONS](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/backoffice/PII/applications)    | [RAW_CPU_UTIL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/postgresql/backoffice/applications) |
| PII_PREPROD | [PII_APPLICATIONS](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/backoffice/PII/applications) | [RAW_CPU_UTIL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/postgresql/backoffice/applications) |


### Структура
| Поле                                 | Описание                                                                |
|--------------------------------------|-------------------------------------------------------------------------|
| id                                   | ID регистрационной формы                                                |
| uuid                                 | 	                                                                       |
| status                               | 	                                                                       |
| created_at_msk                       | Дата и время создания (временная зона MSK (UTC+3:00)) (старое название) |
| created_at_dttm_local                | Дата и время создания (временная зона MSK (UTC+3:00)) (новое название)  |
| created_at_ts                        | timestamp создания                                                      |
| updated_at_msk                       | Дата и время последнего обновления (временная зона MSK (UTC+3:00))      |
| event_id                             | ID события                                                              |
| participant_id                       | ID участника                                                            |
| visited                              | 	                                                                       |
| language                             | 	                                                                       |
| visit_type                           | 	                                                                       |
| participant_name_hash                | Хеш имени участника                                                     |
| participant_last_name_hash           | Хеш фамилии участника                                                   |
| participant_company_name_hash        | Хеш названия компании в которой участник работает                       |
| participant_position_hash            | Хеш должности в компании                                                |
| participant_phone_hash               | Хеш номера телефона участника                                           |
| participant_email_hash               | Хеш почты участника                                                     |
| participant_city                     | Города участника                                                        |
| participant_company_industry         | сфера деятельности компании                                             |
| participant_company_size             | 	                                                                       |
| participant_position_type            | 	                                                                       |
| participant_website                  | 	                                                                       |
| participant_has_yacloud_account      | 	                                                                       |
| participant_scenario                 | 	                                                                       |
| participant_need_manager_call        | 	                                                                       |
| participant_infrastructure_type      | 	                                                                       |
| participant_how_did_know_about_event | 	                                                                       |
| participant_agree_to_communicate     | 	                                                                       |
| participant_utm_source               |                                                                         |
| participant_utm_campaign             |                                                                         |


### PII cтруктура
| Поле                         | Описание                             |
|------------------------------|--------------------------------------|
| id                           | ID регистрационной формы             |
| participant_name             | Имя участника                        |
| participant_last_name        | Фамилия участника                    |
| participant_company_name     | Компания в которой работает участник |
| participant_company_industry | Сфера деятельности компании          |
| participant_position         | Должность в компании                 |
| participant_phone            | Номер телефона участника             |
| participant_email            | Почта участника                      |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: каждый час.

## analytics organizations
#analytics #organizations

Таблица Трекера, данные с 2019-09-09, момент когда на источнике в последний раз менялась схема данных

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                              | Источники                                                                                                                     |
|---------|------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [organizations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/analytics/organizations)               | [raw_organizations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/analytics/organizations)    |
| PREPROD | [organizations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/analytics/organizations) | [raw_organizations](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/analytics/organizations) |


### Структура
| Поле                | Описание                                                                    |
|---------------------|-----------------------------------------------------------------------------|
| admins              | администраторы организации                                                  |
| deputy_admins       | администраторы организации с ограниченными правами                          |
| first_debt_act_date | дата выставления первого акта, по которому была задолженность               |
| organization_id                  | id организации                                                              |
| language            | язык Коннекта                                                               |
| name                | название организации                                                        |
| organization_type   | тип организации. common - обычная, yandex_project - Яндекс                  |
| registration_date   | дата регистрации организации                                                |
| services            | подключенные сервисы                                                        |
| source              | источник регистрации                                                        |
| subscription_plan   | тип подписки                                                                |
| tld                 | национальный домен, на котором была зарегистрирована организация в Коннекте |
| for_date_dttm_local | время снапшота по Москве                                                    |
| for_date_ts         | timestamp снапшота                                                          |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.

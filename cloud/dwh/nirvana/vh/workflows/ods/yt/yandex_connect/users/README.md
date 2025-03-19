## analytics users
#analytics #users

Таблица Трекера, данные с 2019-09-09, момент когда на источнике в последний раз менялась схема данных

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                              | Источники                                                                                                                     |
|---------|------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/analytics/users)               | [raw_users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/analytics/users)    |
| PREPROD | [users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/analytics/users) | [raw_users](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/analytics/users) |


### Структура
| Поле                | Описание                                                 |
|---------------------|----------------------------------------------------------|
| created             | дата регистрации пользователя в Коннекте                 |
| licensed_services   | список сервисов, на которые есть лицензии у пользователя |
| org_id              | id организации пользователя                              |
| uid                 | паспортный uid пользователя                              |
| for_date_dttm_local | время снапшота по Москве                                 |
| for_date_ts         | timestamp снапшота                                       |



### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.

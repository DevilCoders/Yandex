#### Описание

Вычитывает последний (актуальный) снапшот таблицы `participants`. Содержит информацию о puid/login из Yandex.Passport

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/backoffice/participants)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/backoffice/participants)

- `id`             - ID участника в Backoffice
- `puid`           - ID пользователя в Yandex.Passport
- `login_hash`     - Хеш логина пользователя в Yandex.Passport
- `created_at_msk` - Дата и время создания (временная зона MSK (UTC+3:00))


Personally Identifiable Information (PII): [PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/backoffice/PII/participants)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/backoffice/PII/participants)

- `login` - логина пользователя в Yandex.Passport

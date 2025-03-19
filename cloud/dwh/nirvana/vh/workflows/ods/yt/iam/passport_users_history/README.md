#### IAM Passport History Users:
#iam #users #history #PII

История паспортных пользователей в IAM

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Расположение данных PII](#Расположение-данных-PII)
4. [Структура PII.](#Структура-PII)
5. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                                       | Источники                                                                                                                                                                                            |
|---------|-------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [passport_users_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/passport_users_history)        | [raw_passport_users_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/identity/hardware/hardware/default/identity/r3/subjects/passport_users_history)       |
| PREPROD | [passport_users_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/passport_users_history)     | [raw_passport_users_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/identity/hardware/hardware/default/identity/r3/subjects/passport_users_history)    |


### Структура
| Поле                 | Описание                                                                                                                                                                          |
|----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| iam_uid              | идентификатор пользователя в IAM                                                                                                                                                  |
| passport_uid         | идентификатор пользователя в Yandex.Passport                                                                                                                                      |
| passport_login       | логин пользователя в Yandex.Passport                                                                                                                                              |
| language             | - язык пользователя (влияет на email/notifications)                                                                                                                               |
| phone_hash           | хеш от номера телефона пользователя                                                                                                                                               |
| email_hash           | хеш от email пользователя                                                                                                                                                         |
| modified_at          | время модификации пользователя                                                                                                                                                    |
| experiments          | участие в UX экспериментах (флаги, например - asideNavigation)                                                                                                                    |
| created_clouds       | кол-во созданных облаков                                                                                                                                                          |
| cloud_creation_limit | квота на кол-во создаваемых облаков                                                                                                                                               |
| mail_tech            | Технические работы. Уведомления о технических работах и сообщения от службы технической поддержки.                                                                                |
| mail_billing         | Биллинг. Уведомления, связанные с состоянием платёжного аккаунта: про оплату и потребление ресурсов, статусы платёжного аккаунта, использование грантов, пробный период и другие. |
| mail_testing         | Тестирование новых сервисов. Приглашения принять участие в пилотном проекте, альфа-тестировании сервисов.                                                                         |
| mail_info            | Пошаговые инструкции. Информация о пошаговых инструкциях и руководствах, которые помогут эффективнее работать с сервисами Яндекс.Облака.                                          |
| mail_feature         | Новости и предложения. Уведомления о новых сервисах, обновлениях текущих сервисов, скидках и специальных предложениях от Яндекс.Облака.                                           |
| mail_event           | Мероприятия. Приглашения на очные мероприятия и вебинары Яндекс.Облака.                                                                                                           |
| mail_promo           | Предложения от других сервисов Яндекса. Информация о специальных предложениях от других сервисов Яндекса.                                                                         |
| mail_alerting        | Мониторинг. Алерты, настроенные с помощью сервиса Yandex Monitoring.                                                                                                              |


### Расположение данных PII
| Контур  | Расположение данных                                                                                                                            |
|---------|------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [pii_passport_users_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/iam/PII/passport_users_history)     |
| PREPROD | [pii_passport_users_history](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/iam/PII/passport_users_history)  |



### Структура PII
| Поле         | Описание                         |
|--------------|----------------------------------|
| iam_uid      | идентификатор пользователя в IAM |
| phone        | номер телефона пользователя      |
| email        | email пользователя               |
| modified_at  | время модификации пользователя   |



### Загрузка

Статус загрузки: активна

Периодичность загрузки: раз в час.

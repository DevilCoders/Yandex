#### Data Mart Marketing Events

Витрина связанных с маркетингом событий Облака. Учитываются только события, связанные только с **первым** биллинг аккаунтом каждого пасспортного пользователя.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/marketing/dm_marketing_events)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/marketing/dm_marketing_events)

* `event_type` - тип события
* `event_id` - идентификатор события. Формат может отличаться для разных типов события
* `event_time_msk` - время события
* `puid` - [паспортный идентификатор пользователя](../../../../ods/yt/iam/passport_users/README.md)
* `billing_account_id` - идентификатор [биллинг аккаунта](../../../../ods/yt/billing/billing_accounts/README.md)

**Типы событий (event_type)**

- `billing_account_created` - создание первого биллинг аккаунта пользователя. [Источник](../../../../stg/yt/cdm/events/billing/common/README.md)
- `billing_account_first_trial_consumption` - совершение первого потребления биллинг аккаунтом. [Источник](../../../../stg/yt/cdm/events/billing/first_trial_consumption/README.md)
- `billing_account_first_paid_consumption` - совершение первого **платного** потребления биллинг аккаунтом. [Источник](../../../../stg/yt/cdm/events/billing/first_paid_consumption/README.md)
- `event_application` - регистрация на мероприятие. [Источник](../../../../ods/yt/backoffice/applications/README.md)
- `event_visit` - посещение мероприятия. [Источник](../../../../ods/yt/backoffice/applications/README.md)
- `site_visit` - посещение сайта. [Источник](../../../../ods/yt/metrika/visit_log/README.md)

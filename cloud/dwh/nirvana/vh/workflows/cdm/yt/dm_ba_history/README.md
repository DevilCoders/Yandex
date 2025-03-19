#### Billing Account History.

Исторические изменения платежных аккаунтов.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_ba_history)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_ba_history)


- `action` - тип изменения. Пока только "create".
- `billing_account_id` - ID [платежного аккаунта](../../../ods/yt/billing/billing_accounts).
- `billing_account_type` - Тип платежного акканта: individual или company.
- `country_code` - код страны, из которой БА.
- `crm_segment` - сегмент crm.
- `event_dt` - дата, когда произошло изменение, MSK.
- `event_dttm` - дата и время, MSK.
- `is_isv` - участник программы Cloud Boost (как Independent Software Vendor).
- `is_subaccount` - является ли приглашенным другим участником (обычно партнером).
- `is_suspended_by_antifraud` - заблокирован как мошенник.
- `is_var` - является ли партнером.
- `person_type` - оригинальный person_type из платежного аккаунта.
- `yandex_office` - Офис, с которым заключен контракт.

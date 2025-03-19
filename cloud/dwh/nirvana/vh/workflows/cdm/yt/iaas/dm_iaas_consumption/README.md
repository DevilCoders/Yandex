## Infrastructure service group consumption
#dm #iaas #dm_iaas_consumption

Витрина потребления сервисной группы Infrastructure

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                                 | Источники                                                                                                              |
|---------|-------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------|
| PROD    | [dm_iaas_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/yt/iaas/dm_iaas_consumption)    | [dm_yc_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/yt/dm_yc_consumption)    |
| PREPROD | [dm_iaas_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/yt/iaas/dm_iaas_consumption) | [dm_yc_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/yt/dm_yc_consumption) |


### Структура
| Поле                                         | Описание                                                                                                                                                                      |
|----------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| billing_account_id                           | идентификатор [биллинг аккаунта](../../../../ods/yt/billing/billing_accounts)                                                                                                 |
| billing_account_name                         | название биллинг акканута                                                                                                                                                     |
| crm_account_name                             | название crm акканута                                                                                                                                                         |
| billing_account_usage_status                 | статус потребления биллинг аккаунта на дату потребления                                                                                                                       |
| crm_architect_current                        | текущий арихитектор привязанный к биллинг аккаунту                                                                                                                            |
| crm_account_owner_current                    | текущий сейлз привязанный к биллинг аккаунту                                                                                                                                  |
| crm_segment                                  | сегмент биллинг аккаунта на дату потребления                                                                                                                                  |
| crm_segment_current                          | текущий сегмент биллинг аккаунта                                                                                                                                              |
| billing_account_is_var                       | является ли биллинг аккаунт VAR на дату потребления                                                                                                                           |
| billing_account_is_var_current               | является ли биллинг аккаунт VAR в текущий момент                                                                                                                              |
| billing_account_is_subaccount                | является ли биллинг аккаунт сабаккаунтом                                                                                                                                      |
| billing_account_is_lazy                      | если биллинг аккаунт потреблял на дату записи SKU с флагом `sku_lazy=0` то `billing_account_is_lazy=0`                                                                        |
| billing_record_msk_date                      | дата потребления                                                                                                                                                              |
| sku_service_name                             | сервис SKU                                                                                                                                                                    |
| sku_subservice_name                          | сабсервис SKU                                                                                                                                                                 |
| sku_service_subservice_name                  | связка сервиса и сабсервиса SKU                                                                                                                                               |
| sku_name                                     | SKU                                                                                                                                                                           |
| billing_record_origin_service                | оригинальный service (кубер или mdb)                                                                                                                                          |
| billing_record_origin_subservice             | оригинальный subservice (для greenplum)                                                                                                                                       |
| sku_pricing_unit                             | единица измерения SKU                                                                                                                                                         |
| sku_lazy                                     | lazy SKU                                                                                                                                                                      |
| billing_record_pricing_quantity              | количество потребленного SKU                                                                                                                                                  |
| billing_record_total_redistribution_rub_vat  | итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление без НДС. Перераспределена сумма с резервовов на основной продукт (актуально для кубера) |
| billing_record_total_rub_vat                 | итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление без НДС                                                                                 |
| billing_record_credit_monetary_grant_rub_vat | скидка в рублях от денежных грантов без НДС                                                                                                                                   |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.

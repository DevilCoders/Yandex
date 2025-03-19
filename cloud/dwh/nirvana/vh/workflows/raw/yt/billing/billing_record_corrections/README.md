#### Billing records corrections

Дополнительные строки потребления, которые нужно учитывать в построении данных.

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/cloud/billing/analytics_cube/cube_corrections/preprod --link-path //home/cloud-dwh/data/preprod/raw/billing/billing_record_corrections
# prod
yt link --proxy hahn --target-path //home/cloud/billing/analytics_cube/cube_corrections/prod --link-path //home/cloud-dwh/data/prod/raw/billing/billing_record_corrections
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/billing/billing_record_corrections)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/billing/billing_record_corrections)

* `billing_account_id` - id биллинг аккаунта
* `cloud_id` - id облака
* `folder_id` - id фолдера
* `resource_id` - id ресурса

* `date` - дата в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`

* `sku_id` - id SKU
* `cost` - стоимость в валюте `currency`
* `credit`- общая скидка

* `labels_hash`- хеш поля `labels`

* `_added_at` - время добавления записи в формате unix timestamp с округлением до конца часа
* `_added_by` - пользователь, который добавил запись

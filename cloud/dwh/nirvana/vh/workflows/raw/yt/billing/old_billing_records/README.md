#### Old billing records

Потребление сервисов, которые было посчитано до realtime-биллинга

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //home/cloud/billing/exported-billing-tables/billing_records_preprod_enriched --link-path //home/cloud-dwh/data/preprod/raw/billing/old_billing_records

# prod
yt link --proxy hahn --target-path //home/cloud/billing/exported-billing-tables/billing_records_prod_enriched --link-path //home/cloud-dwh/data/prod/raw/billing/old_billing_records
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/billing/old_billing_records)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/billing/old_billing_records)

* `billing_account_id` - id биллинг аккаунта
* `cloud_id` - id облака

* `start_time`- время потребления в формате unix timestamp с округлением до начала часа
* `end_time` - время потребления в формате unix timestamp с округлением до конца часа
* `created_at` - время обработки в формате unix timestamp

* `sku_id` - id SKU
* `pricing_quantity` - размер потребления в единицах `pricing_unit`
* `cost` - стоимость в валюте `currency`
* `credit`- общая скидка
* `credit_charges`- yson-поле со списком примененных скидок

* `labels_hash`- хеш поля `labels`

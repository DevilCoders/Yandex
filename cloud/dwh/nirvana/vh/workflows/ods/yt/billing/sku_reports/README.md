#### Billing SKU Report:

Вычитывает последний (актуальный) снапшот таблицы `sku_reports` - дневная детализация потребления по продуктам (sku) и billing account

##### Схема

[INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/sku_reports)

- `billing_account_id` - идентфикатор платежного аккаунта
- `sku_id` - идентфикатор SKU
- `date` - дата потребления (MSK)
- `pricing_quantity` - кол-во потребления
- `cost` - полная стоимость услуг
- `credit` - скидки
- `created_at` - дата создания строки

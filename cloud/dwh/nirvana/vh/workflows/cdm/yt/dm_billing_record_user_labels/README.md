#### Data Mart Billing Record User Labels

Витрина лейблов и их хешей ресурсов пользователей.

##### Схема

[PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_billing_record_user_labels)
/ [PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_billing_record_user_labels)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/cdm/dm_billing_record_user_labels)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/cdm/dm_billing_record_user_labels)

* `billing_record_user_label_key` - ключ лейбла
* `billing_record_user_label_value` - значение лейбла
* `billing_record_labels_hash` - хэш исходного лейбла, которому принадлежат ключ и значение


##### Пример использования

Запрос, позволяющий по лейблу найти данные.

`$records` - таблица с данными
`$billing_record_user_labels` - таблица с лейблами
`$key` - ключ, который нам интересен
`$value` - значение, которое нам интересно у ключа

```sql
$internesting_hashes = (
    SELECT
        billing_record_labels_hash as hash
    FROM $billing_record_user_labels
    WHERE billing_record_user_label_key = $key AND billing_record_user_label_value = $value
    GROUP BY billing_record_labels_hash
);

SELECT
    records.*
FROM $records AS records
INNER JOIN $internesting_hashes AS hashes
ON (records.hash = hashes.billing_record_labels_hash)
```

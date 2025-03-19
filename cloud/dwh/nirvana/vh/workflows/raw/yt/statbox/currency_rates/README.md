#### Currency rates

Курс валют.

Так как уже данные лежат в yt, то просто настроена ссылка:

```bash
# preprod
yt link --proxy hahn --target-path //statbox/statbox-dict-last/currency_rates.json --link-path //home/cloud-dwh/data/preprod/raw/statbox/currency_rates.json

# prod
yt link --proxy hahn --target-path //statbox/statbox-dict-last/currency_rates.json --link-path //home/cloud-dwh/data/prod/raw/statbox/currency_rates.json

# prod_internal
yt link --proxy hahn --target-path //statbox/statbox-dict-last/currency_rates.json --link-path //home/cloud-dwh/data/prod_internal/raw/statbox/currency_rates.json

# preprod_internal
yt link --proxy hahn --target-path //statbox/statbox-dict-last/currency_rates.json --link-path //home/cloud-dwh/data/preprod_internal/raw/statbox/currency_rates.json
```

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/statbox/currency_rates.json)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/statbox/currency_rates.json)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/raw/statbox/currency_rates.json)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/raw/statbox/currency_rates.json)

Окружения не отличаются. Это не таблица, а файл в json формате.

```json
{
    "2015-01-01": {
        "EUR": 68.3681,
        "KZT": 0.308143,
        "TRY": 24.1789,
        "USD": 56.2376
    },
    ....
}
```

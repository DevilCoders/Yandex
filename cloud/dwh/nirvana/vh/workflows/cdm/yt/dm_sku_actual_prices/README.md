#### Описание

Витрина, в которой показан актуальная стоимость SKU на текущий момент

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_sku_actual_prices)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_sku_actual_prices)

* `sku_id`                  - Идентификатор SKU (Stock Keeping Unit)
* `balance_product_id`      - Идентификатор продукта (услуги) в Яндекс.Баланс
* `publisher_account_id`    - Идентификатор аккаунта, опубликовавшего SKU
* `publisher_account_name`  - Имя аккаунта, опубликовавшего SKU
* `service_id`              - Идентификатор сервиса
* `service_name`            - Краткое имя сервиса
* `service_descripiton`     - Описание сервиса
* `translation_en`          - Описание SKU на английском языке
* `translation_ru`          - Описание SKU на русском языке
* `start_time`              - Время начала действия SKU
* `end_time`                - Время окончания действия SKU
* `rate_id`                 - Уровень определяющий стоимость SKU (`unit_price` может отличаться на разных уровнях)
* `start_pricing_quantity`  - Количество потребленных `pricing_unit` определяющих начало уровня
* `end_pricing_quantity`    - Количество потребленных `pricing_unit` определяющих завершение уровня
* `pricing_unit`            - Единица в которой считается потребление на текущем уровне
* `unit_price`              - Стоимость единицы потребления

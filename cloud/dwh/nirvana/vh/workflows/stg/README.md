#### STG. Staging

Вспомогательные данные -- это временные данные, которые используются в других etl-процессах. Например, предагрегации таблиц для CDM-слоя.

**Не рекомендуется к использованию во внешних процессах, так как данные могут удаляться и менять формат**

#### Структура слоя

Данные в коде хранятся в `/stg/destination_system/destination_layer/entity_name`

* `destination_system` - место хранения данных, например `yt`
* `destination_layer` - слой, для которого создаются вспомогательные данные
* `entity_name` - название сущности с данными

Данные в месте хранения должны лежать в `/stg/destination_layer/entity_name`

##### Таблицы

* [yt.events.billing.common](./yt/cdm/events/billing/common/README.md)
* [yt.events.billing.feature_flag_changed](./yt/cdm/events/billing/feature_flag_changed/README.md)
* [yt.events.billing.first_paid_consumption](./yt/cdm/events/billing/first_paid_consumption/README.md)
* [yt.events.billing.first_payment](./yt/cdm/events/billing/first_payment/README.md)
* [yt.events.billing.first_trial_consumption](./yt/cdm/events/billing/first_trial_consumption/README.md)
* [yt.events.billing.state_changed](./yt/cdm/events/billing/state_changed/README.md)

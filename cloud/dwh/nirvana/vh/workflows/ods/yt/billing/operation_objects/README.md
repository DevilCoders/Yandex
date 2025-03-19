#### Описание

Вычитывает последний (актуальный) снапшот таблицы `operation_objects`

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/operation_objects)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/operation_objects)

* `operations_id` - идентификатор [операции](../operations/README.md)
* `object_type`   - тип, связанной с операцией сущности
* `object_id`     - идентификатор, связанной с операцией сущности
* `object_role`   - роль, в которой выступала сущность (если не совпадает с `object_type`)

#### Data Mart Console Automatic Clicks Events

Витрина событий (кликов) пользователей, которые автоматически собираются в консоли Yandex Cloud.

Счетчик: 48570998

Таблица сформирована на базе [ods](../../../../ods/yt/metrika/hit_log/README.md).

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_console_automatic_clicks_events)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_console_automatic_clicks_events)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/cdm/dm_console_automatic_clicks_events)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/cdm/dm_console_automatic_clicks_events)

* `event_date` - дата события
* `component` - тип компонента, на которой был совершен клик
* `action` - тип события
* `xpath` - xpath компонента в DOM-дереве страницы
* `url` - url (path) страницы, на котором был совершен клик
* `path` - хэш от xpath
* `hit_id` - WatchID события (клика)
* `user_id` - идентификатор пользователя, совершившего клик

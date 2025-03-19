#### Data Mart DataLens Funnel without promo
Витрина с воронкой новых пользователей, пришедших в DataLens, без учета промо-страницы. Воронки состоят из следующих шагов:
* Переход на auth auth.cloud.yandex.ru
* Клик на кнопку "Войти" на auth
* Посещение welcome-страницы DataLens ("Добро пожаловать в DataLens")
* Просмотр любой страницы DataLens (кроме promo и init)

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/datalens/dm_datalens_funnel_without_promo)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/datalens/dm_datalens_funnel_without_promo)

* `event_date` - дата события
* `user_id` - идентификатор посетителя
* `event_type` - тип события (шаг воронки)

#### Data Mart DataLens Funnel with promo
Витрина с воронками новых пользователей, пришедших в DataLens. Воронки состоят из следующих шагов:

Полностью новый пользователь:
* Посещение промо-страницы на https://datalens.yandex.ru/
* Переход на auth auth.cloud.yandex.ru
* Клик на кнопку "Войти" на auth
* Посещение welcome-страницы DataLens ("Добро пожаловать в DataLens")
* Просмотр любой страницы DataLens (кроме promo и init)

Пользователь, новый для DataLens:
* Посещение промо-страницы на https://datalens.yandex.ru/
* Переход на страницу "Войти"
* Открытие или создание нового DataLens
* Просмотр любой страницы DataLens (кроме promo и init)

Таблица сформирована на базе [ods](../../../../ods/yt/metrika/hit_log/README.md).
Используются счетчики 50514217 и 51465824.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/datalens/dm_datalens_funnel_with_promo)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/datalens/dm_datalens_funnel_with_promo)

* `event_date` - дата события
* `user_id` - идентификатор посетителя
* `event_type` - тип события (шаг воронки)

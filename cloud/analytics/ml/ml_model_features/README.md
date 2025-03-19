#### Хранилище фичей для ML-моделей:

Компоненты:
- [Запросы в поиск](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud_analytics/ml/ml_model_features/by_puid/search_requests)
- ...

##### Схема

Фичи разделены на две группы: by_baid (заязанные на `billing_account_id`) и by_puid (завязанные на `passport_uid`). Для последней группы есть возможность добавить puid не связанные с текущими билинг аккаунтами. Для этого puid можно залить в таблицу [COLD_PUIDS](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud_analytics/ml/ml_model_features/by_puid/cold_puids) после чего вся история фичей обновится с учетом этих puid.

##### Тикет 

https://st.yandex-team.ru/CLOUDANA-1759

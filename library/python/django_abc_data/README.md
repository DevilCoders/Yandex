## Как использовать

### Установка

* Установить пакет `django-abc-data`
* Добавить `django_abc_data` в `INSTALLED_APPS`
* Настроить


### Запуск

Есть несколько вариантов запуска:

* management команда `sync_abc_services`
* celery таск `django_abc_data.tasks.sync_services`
* вручную запустить `django_abc_data.core.sync_services`

### Кастомизация и настройка

#### Настройки

| Название                        | Обязательность   | Описание                                      |
|---------------------------------|------------------|-----------------------------------------------|
| ABC_DATA_IDS_USER_AGENT         | Да               | Юзер агент для ABC                            |
| ABC_DATA_IDS_OAUTH_TOKEN        | Нет              | OAuth токен для ABC                           |
| ABC_DATA_TVM_CLIENT_ID          | Нет              | TVM id клиента                                |
| ABC_DATA_YLOCK                  | Только для таска | Настройки ylock                               |
| ABC_DATA_MIN_LOCK_TIME          | Только для таска | Минимальное время блокирования ylock          |
| ABC_DATA_IDS_TIMEOUT            | Нет              | Таймаут для ABC                               |
| ABC_DATA_IDS_PAGE_SIZE          | Нет              | Количество результатов на странице для ABC    |
| ABC_DATA_SYNCER_CLASS_PATH      | Нет              | Кастомный класс синхронизации                 |
| ABC_DATA_API_VERSION            | Нет              | Версия api используемая в abc                 |


#### Свой класс синхронизатора

Можно унаследовать от django_abc_data.syncer.AbcServiceSyncer

### Сигналы

При добавлении и изменении сервисов посылаются сигналы service_created и service_updated соответственно. В них передаётся объект сервиса и data - словарь, содержащий diff.

## Как запустить тесты
* `fab venv`
* `fab test`

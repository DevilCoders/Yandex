# dir-sync

Джанго-приложение для сихнронизации данных Директории в своём сервисе.

# Использование

1. Указать в зависимостях dir-sync.
2. Добавить приложение в INSTALLED_APPS.
3. Включить настройки dir-sync в django-настройки `from dir_data_sync.settings import *`
4. Задать настройки, глядя на `dir_data_sync/settings.py`

Для фоновых задач синхронизации нужны рабочие настройки ylock.

# Changelog

* 0.19 – переход на с
[APIv2](https://api.directory.ws.yandex.ru/docs/api-changelog.html#v2-2017-03-01-yandex-directory-0-166) на
[APIv7](https://api.directory.ws.yandex.ru/docs/api-changelog.html#v7-2017-10-31-yandex-directory-0-195-0) Директории.
Для оптимизации синхронизации можно переопределить настройки `DIRSYNC_FIELDS_USER, DIRSYNC_FIELDS_DEPARTMENT, DIRSYNC_FIELDS_GROUP`,
указав только те поля, которые используются в обработчике сигналов добавления пользователей, департаментов, групп.

# Разработка

Собрать релиз:
1. Изменить версию в setup.py
2. `releaser release-changelog`


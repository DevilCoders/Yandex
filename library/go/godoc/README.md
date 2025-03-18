# godoc

Аркадийная версия godoc.

## Как обновить версию godoc внутри ya tool
1. Склонировать задачу https://sandbox.yandex-team.ru/task/511821444/view
2. В поле `Svn url for arcadia` удалить ревизию (число после @). Если этого не сделать, соберётся старая версия.
3. Дождаться завершения задачи.
4. ID задачи вписать в файл [build/ya.conf.json](https://a.yandex-team.ru/arc/trunk/arcadia/build/ya.conf.json)
5. Локальным запуском `ya tool godoc` проверить что все ок.
6. Закоммитить изменения.

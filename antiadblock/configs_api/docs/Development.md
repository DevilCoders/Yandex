# Development guide  
[Разработка в Аркадии (ссылка в cryprox)](/arc/trunk/arcadia/antiadblock/cryprox/docs/Arcadia.md)

### База данных
* Живем на PGaaS' PostgreSQL 10.4
* [Данные кластера](https://st.yandex-team.ru/ANTIADB-597#1518611335000)
* [Миграции pgmigrate](https://st.yandex-team.ru/ANTIADB-768)
* [Документация по pgmigrate](https://github.com/yandex/pgmigrate/blob/master/doc/tutorial.md)
* [Документация по Yandex Managed Databases](https://doc.yandex-team.ru/cloud/mdb/index.html)
* [Документация на клиент yandex_cloud](https://wiki.yandex-team.ru/MDB/quickstart/)

### Миграции
Для изменения структуры базы данных и миграции используется (pgmigrate)[https://github.com/yandex/pgmigrate/blob/master/doc/tutorial.md]</br>     
На dev-стенды приезжает дамп продовой базы (шаг `postgres-restore`)</br>
Дамп делается раз в сутки [sandbox-таской](https://sandbox.yandex-team.ru/scheduler/12784/view)</br>
и заливается в [YT-LOCKE](https://yt.yandex-team.ru/locke/navigation?path=//home/antiadblock/configs_api/pg_db_dump)</br>
Если нужен свежий дамп данных, то можно запустить таску вручную</br>
На dev-стендах и на проде миграции применяются автоматически, сейчас это делает сервант самогона ./samogon/plugin/\_\_init__.py#PgMigrate</br>
Для того, чтобы написать новую миграцию надо добавить очередной .sql файл в [migrations/migrations]</br>
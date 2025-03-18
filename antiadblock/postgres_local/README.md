Позволяет использовать локальную базу PostgreSQL v.10.5.1 в тестах. 

## Рецепт
Есть простой рецепт, создающий postgres-postgres-postgres. Для использования надо добавить строку в ya.make тестов
`INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/recipe.inc)`

Пример: [/arc/trunk/arcadia/antiadblock/postgres_local/tests_recipe/]

Имя базы, имя пользователя кастомизируются параметрами к рецепту. Вместо директивы INCLUDE, стоит использовать исправленное содержимое [/arc/trunk/arcadia/antiadblock/postgres_local/recipe/recipe.inc]:

```
USE_RECIPE(antiadblock/postgres_local/recipe/recipe --port 5432 --user your_username --db_name your_database)
INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)
DEPENDS(
  antiadblock/postgres_local/recipe
)
```

В тестах можно подключаться к базе данных, используя переменные окружения:
```
@pytest.yield_fixture(scope='session')
def db_connection():
    user = os.environ['PG_LOCAL_USER']
    password = os.environ['PG_LOCAL_PASSWORD']
    db_name = os.environ['PG_LOCAL_DATABASE']
    port = os.environ['PG_LOCAL_PORT']

    db_uri = 'postgresql+psycopg2://{user}:{password}@{host}:{port}/{db_name}'.format(user=user,
                                                                                      password=password,
                                                                                      host='localhost',
                                                                                      port=port,
                                                                                      db_name=db_name)
    engine = None
    try:
        engine = create_engine(db_uri, echo=True)
        with engine.connect() as conn:
            yield conn
    finally:
        if engine is not None:
            engine.dispose()
```

Если есть папка с простыми sql миграциями, то путь от корня можно указать в параметрах рецепта (добавив их как DATA в ya.make):
```
USE_RECIPE(antiadblock/postgres_local/recipe/recipe --port 5432 --user your_username --db_name your_database --migrations_path antiadblock/postgres_local/tests/test_migrations)
INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)
DATA(arcadia/antiadblock/postgres_local/tests/test_migrations)
```
Что-то более сложное придется писать в коде

## Библитека
Для более сложных кейсов можно использовать зависимость через PEERDIR, добавив в ya.make тестов привоз бинарей базы из Сендбокс ресурсов:

```
INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)

PEERDIR(
    antiadblock/postgres_local
)
```
Пример: [/arc/trunk/arcadia/antiadblock/postgres_local/tests]

В ресурсах лежат скаченные с https://www.enterprisedb.com/download-postgresql-binaries официальные бинари базы, собранные под линукс и под мак. 
Заливались на сендбокс с помощью : `ya upload --ttl inf -T=ANSWERS_POSTGRESQL  --arch=linux --owner=ANTIADBLOCK /Users/solovyev/Downloads/pgsql`


В тестах использовать можно примерно так:
```
@pytest.yield_fixture(scope='session')
def postgresql_database():
    with yatest.common.network.PortManager() as pm:
        postgresql = None
        try:
            psql_config = Config(port=pm.get_port(5432),
                                 username='antiadb',
                                 password='postgres',
                                 dbname='configs')
                                 
            migrator = FileBasedMigrator(yatest.common.source_path('antiadblock/postgres_local/tests/test_migrations'))

            postgresql = PostgreSQL(psql_config, migrator)
            postgresql.run()
            postgresql.ensure_state(State.RUNNING)

            yield postgresql
        finally:
            if postgresql is not None:
                postgresql.shutdown()
```

FileBasedMigrator - забирает миграции из папки (должна быть подключена в ya.make тестов через DATA), и применяет их в алфовитном порядке. При желании, можно
написать свой Migrator с кастомной логикой

Если нужны какие-то фичи или хотите что-то доработать, пишите @solovyev


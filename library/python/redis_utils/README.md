Здесь лежит библиотека для удобной работы с Redis в Python (в том числе с Celery)


### Примеры

Создание объекта с настройками

```python
from library.python.redis_utils import RedisSentinelSettings


# Пароль сам возьмется из переменной окружения REDIS_PASSWORD (но можно прокинуть аргументом password в конструктор)
# Прочие аргументы тоже можно брать из переменных (вызывая конструктор без аргументов)
# hosts в переменные окружения нужно передавать в json-формате (REDIS_HOSTS=["man-...","sas-..."])
settings = RedisSentinelSettings(
    hosts=['man-0blu26wo5dj059n6.db.yandex.net'],
    cluster_name='cluster',
)
```

Пример получения [клиента](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/redis/redis/client.py#L519)

```python
master = settings.get_redis_master()
slave = settings.get_redis_slave()

master.set('foo', 'bar')
slave.get('foo')
```

Пример настройки Celery

```python
from celery import Celery
from library.python.redis_utils import RedisSentinelSettings


redis_settings = RedisSentinelSettings()
app = Celery('celery', broker=redis_settings.make_connection_url())
app.conf.broker_transport_options = redis_settings.make_celery_broker_transport_options()
```

С библитекой поставляется рецепт для тестов.
Чтобы работать в тестах с редисом,
добавьте в ямейк тестов `INCLUDE(${ARCADIA_ROOT}/library/python/redis_utils/recipe/recipe.inc)`
и создавайте `RedisSentinelSettings` без аргументов (или с `hosts=["localhost"]`)

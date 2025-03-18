datasources-config-wrapper
===============

Оборачивает пакет datasources. Добавляет возможность переопределить настройки приложения вашим файликом
```local_datasources.py``` на вашей файловой системе.

Кроме того, при обращении к настройке, которой нет в датасорсес, обертка возвращает None. Это позволяет
писать код вот так:

```
# settings.py:

wrapper = datasources_config_wrapper.DatasourcesConfigWrapper(prefix='jing')

STORAGE_URL = wrapper.unset_storage_setting

```

Этот код попытается достать из datasources настройку ```jing_unset_storage_setting```. И в любом случае
что-то вернет.

Так же поддерживает `fallback` на переменные окружения, если в файле
нет нужного ключа, пример использования:

Пусть у нас есть перменная окружения `jing_storage_setting= some_value`
```
# settings.py:

wrapper = datasources_config_wrapper.DatasourcesConfigWrapper(
    prefix='jing',
    fallback_on_env_vars=True,
)

STORAGE_URL = wrapper.storage_setting  # 'some_value'

```

При этом пробует получить и по верхнему регистру тоже, так что
если переменная окружения называется `JING_STORAGE_SETTING`
код выше тоже вернет ее значение

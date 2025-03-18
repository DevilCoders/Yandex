# granular_settings


Простой способ загрузки конфигов для разных окружений.

## Примеры использования

### Классический способ

Исторически granular_settings использовался так:

1. Директория <project>/settings содержит файлы конфигурации.

2. Файл <project>/settings.py:

    from granular_settings import *

3. В любом файле проекта:

    import settings
    print settings.VERSION

### Другие способы

#### Загружать файлы конфигурации из определённой директории

Файл settings.py:

    import granular_settings
    granular_settings.set(globals(), path='/etc/myproject')

#### Загружать файлы конфигурации из директории, указанной в переменной окружения

Файл settings.py:

    import granular_settings
    globals().update(**granular_settings.from_envvar(variable_name='MYPROJECT_SETTINGS'))

#### А-ля django

Файл conf.py:

    import granular_settings
    settings = granular_settings.from_envvar(variable_name='MYPROJECT_SETTINGS')

В файлах проекта:

    from myproject.conf import settings
    print settings.VERSION


См. также примеры использования в директории examples.

### Known Issues

Классический способ использовальзования

    from granular_settings import *
    
ломает дебаггер последней версии PyCharm'а из-за магии автоопределения пути до директории с конфигами.
Используйте явное указание пути

    import granular_settings
    granular_settings.set(globals(), path='/etc/myproject')

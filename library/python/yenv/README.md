`yenv` предоставляет удобный доступ к текущему окружению системы (`yandex-environment-*`). 
Окружение представляет собой два значения - тип (`type`) и имя (`name`). 
Самые распространенные типы это `development`, `testing` и `production`.

Источниками информации об окружении, которые может использовать `yenv`, являются:

  * Переменные окружения `YENV_TYPE` и `YENV_NAME`
  * Файлы в директории `/etc/yandex/` `environment.type` и `environment.name`

Пример использования библиотеки:

    >>> import yenv

    >>> yenv.type
    development

    >>> yenv.name
    intranet

Также для упрощения работы с окружениями доступны вспомогательные функции:

    >>> yenv.choose_type(['development', 'testing', 'production'])
    development

    >>> d = {'development': 1, 'testing': 2}
    >>> yenv.choose_key_by_type(d)
    1

Для популярных окружений в yenv существует список фолбеков 
(словарь `yenv._fallbacks`). Например, 
для окружения `development` фолбеком являются `testing > prestable > production`.
Это нужно, чтобы не дублировать одинаковые значения для разных окружений. Для 
этого во все вспомогательные функции можно передать параметр `fallback=True`

    >>> yenv.choose_type(['testing', 'production'], fallback=True)
    testing 
    
    >>> yenv.choose_key_by_type(
        {
            'testing': 'service.test.yandex-team.ru',
            'production': 'service.yandex-team.ru',
        }, 
        fallback=True,
    )
    'service.test.yandex-team.ru'
    
Последний шорткат можно переписать с использованием шортката `choose` 
или `choose_kw`
    
    >>> yenv.choose({
        'testing': 'service.test.yandex-team.ru',
        'production': 'service.yandex-team.ru',
    })
    'service.test.yandex-team.ru'
    
    >>> yenv.choose_kw(
        testing='service.test.yandex-team.ru',
        production='service.yandex-team.ru',
    )
    'service.test.yandex-team.ru'

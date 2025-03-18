# tools_structured_logs

Не привязанные к реализации структурированные логи https://wiki.yandex-team.ru/content/logging/

## Библиотека предоставляет

Интерфейс, позволяющий писать в заданном формате логи от разных `поставщиков`, с временами ответа этих поставщиков и заданным форматом fields.   


## Для чего:

Например можно инструментировать следующих поставщиков:
* Django Orm
* requests
* SQLAlchemy 
* все что угодно

### Сбор времен ответов

После корректной настройки времена ответа вашего приложения появятся тут https://stat.yandex-team.ru/Tools/Logs/endpoint_performance

Есть еще набор разовых YQL запросов:

  * HTTP запросы с группировкой по endpoint упорядоченные в порядке от более проблемных к менее, [Пример над бекэндом Вики](https://yql.yandex-team.ru/Operations/WwFIlfvJNQmd766GhZp8IguD380FCTXXQVSPVUQW8AY=), TODO: срез по uid, срез по oAuth, TVM приложению
  * HTTP запросы в эндпоинт с группировкой по uid упорядоченные по медиане времени исполния. [Анализ кукера](https://yql.yandex-team.ru/Operations/WbliISRHwc7k_GFY--KxF0EnDqXSBglPz2UmHJzqkpY=)
  * Анализ запросов производимых вьюхой, [Сохранение страницы](https://yql.yandex-team.ru/Operations/5b1a7420fbc935d9c7ee648f). Видны HTTP и SQL запросы упорядеченные от самых "влиятельных" к самых "невлиятельным"
  * Исследовать конкретную view в заданном промежутке. [Просмотр страницы в Вики](https://yql.yandex-team.ru/Operations/WsINJiK7YkjSggHen4rjLKp0hCLsFSKiaKb3ZDk7Gno=), [График](https://charts.yandex-team.ru/preview/editor/YQL/charts/5b061493650e6f762781dc13/1)
  * Исследовать конкретный вызов некоторой View. [Поиск по комитом dogma](https://yql.yandex-team.ru/Operations/WenprR1icVQR9Drd_OxXFyH8f92lIjlrOxnRbGC_9MU=)
  * [Все ошибки в ручке](https://yql.yandex-team.ru/Operations/WvQiGW7BhBf8RtIXw1J6Qhwsk2juuCpOsbHypagtwxU=)

# Сбор потребителей

По таким логам удобно вычислять наиболее активных потребителей сервиса через YQL по следующим срезам: по uid, по типу: oauth, tvm, sessionid, по ручке (endpoint) в АПИ, по отрезку времени, по доле от общего числа запросов (`GROUP BY user.uid` или `GROUP BY auth.application`, если речь про TVM, oAuth).

## Использование

См. директорию `helpers`.

### django
Cм библиотеку [django_tools_log_context](https://a.yandex-team.ru/arc/trunk/arcadia/library/python/django_tools_log_context/)

### flask
См как это реализовано в коде [Директории](https://abc.yandex-team.ru/services/directory/). Спросите у команды сервиса.

## Терминология

### Вендоры
`logic/log_records/vendors`. Поставщики информации про конкретные источники: http, sql и другие

### Общие поля логов 
`logic/common_log_fields`. Контекст, который задает структуру fields, как описано на https://wiki.yandex-team.ru/content/logging/.
Каждое поле описывается наследником `logic/common_log_fields/base:Provider`.
### Слой конфигурации
Набор абстрактных классов, который нужно доопределить в своем проекте:
* Config
* Logger
* InstrumentedApplicationHooks

Если все сконфигурировано правильно, достаточно выполнить два шага:
1. Вызвать get_library при старте приложения.
2. В dispatch в своей базовой вьюхе написать код:
```(python)
with InstrumentedApplicationHooks_instance.all_the_logs_for_http(request, threshold):
  return super(MyView, self).dispatch(*args, **kwargs)
```

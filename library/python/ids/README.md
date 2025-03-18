## Описание
**Intranet Data Sources** — библиотека, позволяющая в едином стиле получать
данные от различных сервисов интранета.

**Вики:** https://wiki.yandex-team.ru/Intranet/Dev/ids

##Установка
Для добавления зависимости в библиотеке нужно добавить в setup.py

```python
setup(
    ...
    install_requires=[
        'ids==1.0.0'
        ...
    ]
    ...
)
```

При установке понадобятся debian-пакеты `libffi-dev` и `libssl-dev`.
Чтобы валидация внутренних сертификатов работала, нужно поставить на
машину debian-пакет `yandex-internal-root-ca`.


Разные сервисы могут потребовать установки разных зависимостей, тогда
нужно написать

```python
install_requires=[
    'ids[staff,startrek2]==1.0.0'
    ...
]
```

Аналогично в `requirements.txt` своего сервиса можно добавить один из следующих
вариантов
```python
ids[staff]==1.0.0
ids[startrek2,plan]==1.0.0
git+git://github.yandex-team.ru/tools/ids.git@mybranch#egg=ids[startrek2]
```

Последний вариант полезен, если в ветке реализована новая функциональность и
нужно проверить правильно ли она работает на сервисе


##Использование
Общий интерфейс к апи разных сервисов обычно выглядит следующим образом.
Сначала нужно получить репозиторий (коллекцию) для объектов нужного типа.

```python
from ids.registry import registry
repo = registry.get_repository('jira', 'tickets', user_agent='myservice', **params)
```

В `params` передаются дополнительные параметры, например, для многих сервисов
нужен параметр `oauth_token`.

Интерфейс репозитория обычно имеет несколько публичных методов, основные из них:

```python
repo.getiter(lookup, **options)  # итератор по отфильтрованным объектам коллекции
repo.get(lookup, **options)  # список отфильтрованных объектов (вычитывает `getiter`, если он является генератором)
repo.get_one(lookup, **options) # получить один объект
repo.get_nopage(lookup, **options) # получить список объектов без паджинации (только в static api)
```
В `lookup` ожидается `dict` с параметрами для фильтрации данных, он
специфичен для каждого сервиса.

Примеры работы можно найти в каталоге `examples`.

## Сервисы

### Склонятор
**service** inflector
**repos** inflector

Методы
* `inflect_string(word, case, inflect_like_fio)` - просклонять word. Возвращает unicode. При inflect_like_fio пытается просклонять как ФИО без разметки.
* `inflect_person(person, case)` - просклонять ФИО. person может быть диктом с ключами first_name, last_name, middle_name, gender, объектом с такими же ключами или строкой.
Ключ middle_name необязателен. Ключ gender может принимать значения 'm', 'f', по умолчанию равен 'm'.
* `get_string_inflections(word, inflect_like_fio)` - просклонять word, возвращает dict-like resource (case_id -> unicode).
* `get_person_inflections(person, inflect_like_fio)` - просклонять person, возвращает dict-like resource (case_id -> dict).

Объекты класса InflectorResource  поддерживают индексацию по полному падежу, сокращенному падежу или падежному вопросу.
Объекты класса TitleResource поддерживают форматирование
* `format(fmt_string, case)` - fmt_string строка с плейсхолдерами {first_name}, {last_name}, {middle_name}

Примеры использования в examples/inflector.py

### Этушка
**service** at
**repos** club, comment, post, user, utils

### Календарь
**service** calendar
**repos** holidays

### Вики-форматтер
**service** formatter
**repos** formatter

В `ids_formater.api` есть методы

 * `convert_to_html(self, wiki_text, config='external', **params)` — превратить wiki-разметку в html
 * `get_structure(self, wiki_text, config='external', **params)` — получить структуру
 распознанной вики-разметки, чтобы извлечь из нее нужные данные
 (например логины упомянутых сотрудников или тикеты в трекерах).

### Старый гэп (deprecated)
**service** gap
**repos** absences

```python
>>> from ids.registry import registry
>>> repo = registry.get_repository('gap', 'absences', user_agent='myservice', token='teamcity')
>>> repo.get({
...     'period_from': '2013-01-01',
...     'period_to': '2014-01-01',
...     'login_or_list': 'desh'
... })
...
... [
...     Absence(date_from=datetime.date(2013, 2, 23), ...),
...     Absence(date_from=datetime.date(2013, 5, 20), ...),
... ]
>>> repo.filter(
...     period_from='2011-01-01',
...     period_to='2013-01-01',
...     login_or_list=['desh', 'thasonic']
... )
...
... {
...     'desh': [
...         Absence(date_from=datetime.date(2013, 2, 23), ...),
...         Absence(date_from=datetime.date(2013, 5, 20), ...),
...     ],
...     'thasonic': [...],
... }
```
Для апи нужен токен https://beta.wiki.yandex-team.ru/staff/gap/api/#proavtorizaciju


### Гэп
**service** gap2
**repos** gaps

```python
>>> from ids.registry import registry
>>> repo = registry.get_repository('gap2', 'gaps', user_agent='myservice', oauth_token='AQAD-****')
>>> repo.get_one({'gap_id': 434143})
... {u'comment': u'test test',
... u'date_from': u'2018-02-19T00:00:00',
... u'date_to': u'2018-02-20T00:00:00',
... u'full_day': True,
... u'id': 434143,
... u'person_login': u'sibirev',
... u'to_notify': [],
... u'work_in_absence': False,
... u'workflow': u'absence'}
...
>>> repo.get({'date_from': '2018-02-19', 'date_to': '2018-02-23'})
... [{u'comment': u'test test',
... u'date_from': u'2018-02-19T00:00:00',
... u'date_to': u'2018-02-20T00:00:00',
... u'full_day': True,
... u'id': 434143,
... u'person_login': u'sibirev',
... u'to_notify': [],
... u'work_in_absence': False,
... u'workflow': u'absence'}]
```
Аутентификация по OAuth


### Jira
**service** jira
**repos** tickets, comments

Для работы с jira нужен корректный oauth2 access_token.

### Орандж
**service** orange
**repos** —

Не имеет репозиториев. В `ids_orange.api` есть методы

 * push_notification(oauth_token, user_agent='myservice', description, target_uids, **params)
 * delete_notification(oauth_token, user_agent='myservice', id, uid=None, **params)

Документация https://beta.wiki.yandex-team.ru/Intranet/orange/api/
Аутентификация по OAuth

### Планер
**service** plan
**repos** services, service_bundles, projects, roles, role_scopes, directions

Для получения всех (или отфильрованных) объектов рекомендуется использовать
метод `getiter` для доступа к объектам репозитория. Он возвращает
итерабельный объект `ResultSet`, при итерировании по которому отдаются
объекты. Обработка постраничного вывода происходит лениво и автоматически.
В качестве альтернативы можно использовать метод `ResultSet.get_pages`,
который возращает итератор по страницам api. Каждая страница является
объектом класса `Page`, который имеет метод `get_next` и атрибут `has_next`, а
также атрибуты `page`, `pages`, `limit`, `total`, `links`. Итерирование по
объекту `Page` возвращает сущности на странице.
`ResultSet` также имеет атрибут `pages` и `total`.

Аутентификация по OAuth


### Стафф
**service** staff
**repos**   person, room, table, group, groupmembership, equipment,
            office, organization, position, occupation, geography

Документация https://staff-api.yandex-team.ru/v3/
Репозитории имеют интерфейсы идентичные планерным.

Аутентификация по OAuth и TVM2

### Стартрек
**service** startrek
**repos** issues, issue_events, issue_types, comments, links, permissions, priorities, queues

Аутентификация по OAuth

### Стартрек api v2
**service** startrek2
**repos** users, queues, issues, issue_types, priorities,
  groups, statuses, resolutions, versions,
  components, applications, linktypes, fields,
  screens, worklog

Аутентификация по OAuth

### Цели
**service** goals
**repos** goals

Аутентификация по OAuth

### Поиск
**service** intrasearch
**repos** suggest

Аутентификация по OAuth или tvm2 user ticket


### Директория
**service** directory
**repos** user, organization

Аутентификация по токену Директории или tvm-тикету.

[examples/directory.py](examples/directory.py)


## Helpers
### oauth
```python
from ids.helpers import oauth

oauth.get_token_by_uid(...)
oauth.get_token_by_password(...)
oauth.get_token_by_sessionid(...)
oauth.get_token_by_token(...)
oauth.get_token_by_code(...)
```

### tvm
```python
from ids.helpers import tvm

tvm.get_ticket_by_client_credentials(...)
```

## Разработка
### Развёртывание virtualenv
`
$ fab venv
`

### Запуск тестов
`
$ fab test
`

Запуск тестов без интеграционных:
`
$ fab test:mode=unit
`

### Релиз
Для того, чтобы собрать новый релиз нужно

1. Увеличить версию setup.py — поможет
утилита https://github.com/peritus/bumpversion:
`bumpversion --current-version 0.6.0 minor setup.py` (станет 0.7.0)
`bumpversion --current-version 0.6.0 patch setup.py` (станет 0.6.1)

2. Закоммитить новую запись в debian/changelog с помощью утилит dch или git dch

Дальше релиз автоматически соберется на TeamCity

## Миграция на новые версии
### 1.3.26
Для использования intrasearch или startrek2 теперь обязательно должна быть
задана YENV_NAME (значения `intranet` или `other`).

### 1.3.16
Для использования oauth теперь обязательно должна быть задана YENV_NAME
(значения `intranet` или `other`).

### 1.2.0
  Теперь в зависимостях `requests[security]`, для сборки ids нужно
добавить в build-depends `libffi-dev` и `libssl-dev`, также нужно добавить
`yandex-internal-root-ca` в runtime depends своего приложения

### 1.1.19 -> 1.1.20
  Несовместимое изменение – нужно передавать строковый идентификатор потребителя
  при получении инстанса репозитория в аргументе user_agent. Пример:
  ```
  employees_repo = registry.get_repository(
      service='staff',
      resource_type='person',
      user_agent='wiki',
      oauth_token=oauth_token,
  )
  ```

### 0.x -> 1.x
  1. Убедиться, что нигде нет прямых импортов репозиториев (они должны
    делаться через `registry.get_repository`)
  2. Заменить в зависмостях записи вида

  ```
  ids-jira==0.11.10
  ids-plan==0.11.10
  ids-staff==0.11.10
  ids-startrek2==0.11.10
  ids==0.11.10
  ```

  на

  ```
  ids[jira,plan,staff,startrek2]==1.0.0
  ```

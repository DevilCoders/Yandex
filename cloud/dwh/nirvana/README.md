### Nirvana + Reactor deployer

Для деплоинга новых workflow в [Nirvana](https://nirvana.yandex-team.ru)
используется [valhalla](https://a.yandex-team.ru/arc/trunk/arcadia/nirvana/valhalla/docs/reference/index.md) (aka vh).
Для деплоинга в [Reactor](https://reactor.yandex-team.ru) используется офицальный клиент.

#### Основные ссылки

* Проект в Nirvana ([UAT](https://nirvana.yandex-team.ru/project/cloud_dwh)
  | [PREPROD](https://nirvana.yandex-team.ru/project/yc_dwh_preprod)
  | [PROD](https://nirvana.yandex-team.ru/project/yc_dwh_prod))
* Директория в Reactor ([UAT](https://reactor.yandex-team.ru/browse?selected=3999314)
  | [PREPROD](https://reactor.yandex-team.ru/browse?selected=8464458)
  | [PROD](https://reactor.yandex-team.ru/browse?selected=8464779))

*UAT - старое окружение, функционал которого должен быть перенесен в PROD*

#### Структура репозитория

* [`deployer`](deployer/README.md) - CLI для деплоинга workflow в Nirvana и reaction в Reactor
* [`config`](config/__init__.py) - базовый класс `BaseDeployConfig` для конфигурации deployer
* [`reactor`](reactor/__init__.py) - обертка над клиентом Reactor
* [`vh`](vh/README.md) - каталог с данными для deployer

* [`common`](common) - общие константы и методы
    * [`operations`](common/operations.py) - константные id операций из Nirvana и их обертки
* [`workflows`](workflows) - модули, которые описывают создание workflow и reaction
    * [`events`](workflows/events/README.md) - [генерация событий](workflows/events/README.md)

#### Деплоинг

Для деплоинга используется [deployer CLI](../deployer/README.md)

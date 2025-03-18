# Тасклеты Паспорта

## Как добавить?

См. [README](https://a.yandex-team.ru/arc/trunk/arcadia/passport/backend/tasklets/README.md).

## Список тасклетов

### Changelog

Собирает ченжлог

#### Вход:
1) `config`
    * `version_prefix` - префикс версии (`.` - единственный разделитель внутри версии)
    * `special_version` - специальная версия (отделяется от версии флоу через `-`)

#### Выход:
1) `result`:
    * `content` - список строк ченжлога
    * `issues` - список строк со слагами тикетов в Стартреке
    * `version` - версия пакета


### CreateConductorTicket

Создаёт кондукторный тикет

#### Вход:
1) `ticket`:
    * `branch` - ветка, куда нужно выложить (`unstable|testing|prestable|stable|hostfix|rollback`)
    * `package` - имя пакета (обязательный)
    * `version` - версия пакета (обязательный)
    * `comment` - описание тикета
    * `do_not_autoinstall` - не выкладывать автоматически (по умолчанию false)

#### Выход:
1) `result`:
    * `key` - номер созданного тикета
    * `url` - ссылка на созданный тикет


### WaitForConductorTicket

Ожидает окончания выкладки указанного кондукторного тикета

#### Вход:
1) `config`:
    * `ticket_key` - номер тикета
    * `time_to_wait` - сколько секунд ждать окончания выкладки тикета

#### Выход:
нет


### CreateStartrekIssue

Создаёт тикет в Стартреке

#### Вход:
1) `issue`:
    * `queue` - очередь (по умолчанию PASSP)
    * `assignee` - исполнитель (по умолчанию тот, кто запустил флоу)
    * `summary` - заголовок тикета (обязательный)
    * `description` - тело тикета
    * `type` - тип тикета (по умолчанию Task)
    * `components` - компоненты

#### Выход:
1) `result`:
    * `key` - ключ созданного тикета
    * `url` - ссылка на созданный тикет


### CreateStartrekReleaseIssue

Создаёт релизный тикет в Стартреке

#### Вход:
1) `issue`:
    * `queue` - очередь (по умолчанию PASSP)
    * `package_name` - имя выкладываемого пакета
    * `package_version` - версия выкладываемого пакета
    * `changelog_lines` - строки ченжлога
    * `components` - компоненты

#### Выход:
1) `result`:
    * `key` - ключ созданного тикета
    * `url` - ссылка на созданный тикет


### TransitStartrekIssues

Меняет статус одного или нескольких тикетов в Стартреке

#### Вход:
1) `config`:
    * `issues` - список ключей тикетов
    * `status` - желаемый статус
    * `resolution` - желаемая резолюция (если требуется для статуса)

#### Выход:
нет


### RunTests

Запускает Python-автотесты в Sandbox

#### Вход:
1) `env` - окружение, где запускать тесты (см. [варианты](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/passport/passport_autotests/__init__.py?rev=r8697256#L9))
2) `projects` - список проектов, для которых надо запускать тесты (список имён поддиректорий в [autotests](https://a.yandex-team.ru/arc/trunk/arcadia/passport/backend/qa/autotests))
3) `send_report_to_st_issue` - ключ тикета, куда нужно запостить (комментом) отчёт о прохождении тестов

#### Выход:
нет


### RunAquaTests

Запускает Java-автотесты в Aqua

#### Вход:
1) `config`:
    * `pack_id` - id из [Аквы](https://aqua.yandex-team.ru/#/packs)
    * `retries` - число попыток запуска тестов (включая первый запуск)
    * `time_to_wait` - время, за которое должен завершиться запуск тестов (включая построение allure-отчёта)
    * `poll_interval` - интервал между попытками узнать статус тестов
    * `send_report_to_st_issue` - ключ тикета, куда нужно запостить (комментом) отчёт о прохождении тестов

#### Выход:
нет


### BuildTvmauthPypi

Собирает tvmauth для pypi под все нужные версии python

#### Вход:
1) `path` - путь к каталогу в Аркадии
2) `target_platform` - win, linux, all и тд
3) `target_python` - для каких версий системного python надо собирать: 3.9, 3.8, all и тд
4) `vault_access_key_record` - логин для загрузки артефакта в pypi
5) `vault_secret_key_record` - пароль для загрузки артефакта в pypi

#### Выход:
нет


### SetSandboxResourceAttribute

Выставляет атрибуты на SB-ресурс

#### Вход:
1) `resource_id` - номер SB-ресурса
2) `attributes` - словарь с атрибутами которые будут добавлены к ресурсу

#### Выход:
нет


### Run Teamcity build

Запускает сборку в teamcity

#### Вход:
1) `teamcity_api.url` - URL к teamcity
2) `build_type.id` - идентификатор сборки в teamcity
3) `build_type.properties` - словарь со свойствами сборки: перечень поддерживаемых свойств надо смотреть в teamcity

#### Выход:
1) `build.url` - ссылка на сборку
2) `build_artifacts` - набор получившихся артефактов
3) `build_artifacts.name` - название артефакта
4) `build_artifacts.url` - URL артефакта

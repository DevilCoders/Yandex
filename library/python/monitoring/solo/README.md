# SOLO #

* [Введение](#quickstart-guide)
* [Quickstart Guide](#quickstart-guide)
    * [Этап описания конфигурации объектов](#etap-opisaniya-konfiguracii-obuektov)
        * [Описание объектов Solomon](#opisanie-obuektov-solomon)
        * [Описание объектов Juggler](#opisanie-obuektov-juggler)
        * [Описание объектов Yasm (beta)](#opisanie-obuektov-yasm)
    * [Этап применения конфигурации объектов](#etap-konfiguracii-obuektov)
        * [State, local_id и provider_id, удаление объектов](#state-local_id-i-provider_id-udalenie-obuektov)
        * [Особенности работы с Solomon](#osobennosti-raboty-s-solomon)
        * [Особенности работы с Juggler](#osobennosti-raboty-s-juggler)
        * [Особенности работы с Yasm (beta)](#osobennosti-raboty-s-yasm)
        * [Особенности работы с Monitoring](#osobennosti-raboty-s-monitoring)
    * [Реализация CLI](#realizaciya-cli)
    * [Примеры](#primery)
* [Эксплуатация](#ekspluataciya)
    * [Описание объектов](#opisanie-obuektov)
    * [Релизные процессы](#reliznye-processy)

Если оглавление не работает - стоит попробовать включить YFM-markdown в настройках.

---

## Введение ##

SOLO - это infrastructure as code py23-библиотека для создания, модификации и удаления объектов в системах мониторинга Яндекса (Solomon, Monitoring и Juggler).

---

## Quickstart guide ##

Работа с библиотекой состоит из 2 этапов - этапа описания конфигурации объектов и этапа взаимодействия с API систем мониторинга.

### Этап описания конфигурации объектов ###

На этом этапе пользователь создает описания конфигурации объектов, которыми он планирует управлять через SOLO - так называемые "локальные" объекты. Для создания локальных объектов SOLO предоставляет несколько наборов классов, соответствующих реальным типам объектов в разных системах мониторинга. Пользователь сам решает, хочет ли он создавать объекты непосредственно в коде или ему больше нравится вариант с хранением конфигурации в JSON и ее подгрузке в рантайме - в документационных примерах же рассмотрен сценарий создания объектов в коде.

#### Описание объектов Solomon ####

Solomon имеет две версии публичного API - v2 и v3, поэтому SOLO предоставляет два набора классов работы с объектами Solomon - [v2](https://a.yandex-team.ru/arc_vcs/library/python/monitoring/solo/objects/solomon/v2) и [v3](https://a.yandex-team.ru/arc_vcs/library/python/monitoring/solo/objects/solomon/v3). **На данный момент v3 API не является стабильным и не рекомендуется к использованию в production.**

Для удобного описания селекторов рекомендуется воспользоваться классом Sensor - он умеет корректно сериализовываться в формат селекторов Solomon.

```python
import datetime

from library.python.monitoring.solo.objects.solomon.sensor import Sensor
from library.python.monitoring.solo.objects.solomon.v2 import Alert, Type, Expression
from library.python.monitoring.solo.objects.juggler import Check


alert = Alert(
    id="hahn_adfox_disk_space_quota",
    project_id="adfox",
    name="disk_space quota for account adfox on cluster hahn",
    annotations={
        "host": "adfox",
        "service": "hahn_adfox_disk_space_quota"
    },
    type=Type(
        expression=Expression(
            program="""
                let percentage = avg({0}) / avg({1});
                let lastValue = to_fixed(percentage, 2);
                alarm_if(percentage > 0.9);
                """.format(
                    Sensor(project="yt", cluster="hahn", service="accounts", medium="default", sensor="disk_space_in_gb", account="adfox"),
                    Sensor(project="yt", cluster="hahn", service="accounts", medium="default", sensor="disk_space_limit_in_gb", account="adfox")
                )
        )
    ),
    notification_channels={
        "juggler_сhannel"
    },
    window_secs=300, delay_seconds=60
)
```

#### Описание объектов Juggler ####

Для описания объектов Juggler SOLO использует классы, предоставляемые библиотекой juggler_sdk. Для единообразия эти классы рекомендуется импортировать [из прокси-библиотеки](https://a.yandex-team.ru/arc_vcs/library/python/monitoring/solo/objects/juggler).

```python
import datetime

from library.python.monitoring.solo.objects.juggler import Check


alert = ... # Solomon alert

check = Check(
    # host and service "connect" check with alert
    host=alert.annotations["host"],
    service=alert.annotations["service"],
    namespace="adfox",
    tags=[
        "sms",
        "telegram"
    ],
    refresh_time=60,
    ttl=600,
    aggregator="logic_or"
)
```

#### Описание объектов Yasm ####

Полный список классов для работы с объектами Yasm можно найти [тут](https://a.yandex-team.ru/arc_vcs/library/python/monitoring/solo/objects/yasm).

```python
from library.python.monitoring.solo.objects.yasm import YasmAlert, ValueModify, JugglerCheck

adfox_engine_anon_usage = YasmAlert(
    name="adfox.engine.anon_usage",
    tags=dict(
        itype=["deploy"],
        deploy_unit=["engine"],
        stage=["adfox-engine-preproduction-1",
               "adfox-engine-production-1",
               "adfox-engine-production-2",
               "adfox-engine-production-3"]
    ),
    signal="mul(div(portoinst-anon_usage_gb_txxx,div(portoinst-memory_limit_gb_tmmv,counter-instance_tmmv)),100)",
    value_modify=ValueModify(type="max", window=600),
    warn=[80, 90],
    crit=[90, None],
    mgroups=["ASEARCH"],
    juggler_check=JugglerCheck(
        host="adfox.engine",
        service="anon_usage"
    ),
    disaster=False
)
```

### Этап конфигурации объектов ###

Для работы с объектами через API SOLO предоставляет класс Controller, который использует специализированные Handler-классы для взаимодействия с различными системами. Controller обрабатывает локальные объекты согласно следующей логике:
* группирует переданные объекты согласно Handler-классам, используемым для их обработки, далее для каждой группы с фиксированным Handler:
  * получает текущее состояние объектов в системе мониторинга
  * показывает разницу между реальным и локальным состоянием объектов
  * если выставлен флаг сохранения изменений:
    * создает отсутствующие в системе объекты, модифицирует отличающиеся объекты
    * по отдельному флагу удаляет "неотслеживаемые" объекты, [подробнее](#state-local_id-i-provider_id-udalenie-obuektov)
    * обновляет соответствие local_id и provider_id в State, [подробнее](#state-local_id-i-provider_id-udalenie-obuektov)

В случае различного рода ошибок при работе с API систем мониторинга SOLO пытается повторно выполнить запрос, если это имеет смысл. Если повторное выполнение запроса не имеет смысла (например, при ответе "Access denied"), или же произошла неожиданная ошибка в коде SOLO, то текущий шаг в логике обработки будет прерван, дальнейшие шаги выполнены не будут и State не будет модифицирован результатами этого шага (однако результаты предыдущих шагов будут уже в нем будут сохранены).

Для работы Controller требуются OAuth-токены - тут крайне рекомендуется использовать [токен SOLO OAuth-app](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=2f9d7a6b369d4cb1b331e3985b99ed60). При необходимости пользователь может также указать отдельные токены для различных внешних систем - подробное описание вариантов можно найти в docstring метода `Controller.__init__`.

```python
import datetime

from library.python.monitoring.solo.controller import Controller


alert = ... # Solomon alert
check = ... # Juggler check

controller = Controller(
    save=True,
    solo_token="...",
    juggler_mark="unique_identifier"
)
controller.process([alert, check])
```

#### State, local_id и provider_id, удаление объектов ####

Не все внешние системы предоставляют возможность пользователю самостоятельно указывать идентификаторы для объектов - соответственно, имеет смысл рассматривать два вида идентификаторов:
1) local_id - локальный идентификатор, используемый для идентификации объектов в коде
2) provider_id - идентификатор, присвоенный системой мониторинга объекту

Для хранения соответствия между local_id и provider_id используется класс State. Пользователь неявно ответственнен за задание local_id - он заполняет поля id + project_id для объекта Solomon / host + service для объекта Juggler, а Controller на основании этих полей создает для объекта local_id. Пользователь никак не взаимодействует с provider_id, его использует только Controller для связи локальных объектов с реальными объектами.

На основании State Controller предоставляет возможность удалить untracked-объекты ("неотслеживаемые", т.е. более не управляемые через SOLO объекты) - для этого необходимо создать Controller с флагом `delete_untracked=True`, если же пользователь не хочет удалять реальные объекты, но хочет почистить State - следует воспользоваться флагом `clear_untracked_in_state=True`.

SOLO предоставляет несколько реализаций State - NoneState, LocalFileState, LockeNodeState.
* NoneState - "не использовать State" - не хранит никакой информации про provider_id, подходит только для работы с системами, разрешающими пользователю задавать идентификатор объекта (Juggler и Solomon), т.к. для таких систем provider_id определяется из local_id;
* LocalFileState - хранит информацию в файле на ФС, подходит для сценария редких изменений конфигурации, в таком случае конфигурацию можно хранить в репозитории;
* LockeNodeState - хранит информацию в ноде Кипариса в Locke, подходит для сценария частых изменений конфигурации - вся работа со стейтом (т.е. все изменения) происходит в условиях захваченного лока;

#### Особенности работы с Solomon ####

`local_id = "{project_id}$${object_type}$${id}"`

В Solomon есть ограничение на уникальность идентификатора - он должен быть уникален в рамках одного проекта для одного класса объектов - поэтому local_id включает в себя project_id и object_type.

Можно использовать NoneState, поскольку Solomon разрешает пользователю задавать идентификаторы объектов.

В SOLO есть ограничение на удаление объектов Solomon - SOLO не удаляет объекты типа Shard, Cluster, Service, Project - пользователь должен будет удалять их через UI при необходимости. Это ограничение было добавлено с целью обезопасить пользователя от случайных удалений важных данных - остальные объекты можно легко восстановить из их описания в SOLO, а метрики в шардах будут удалены безвозвратно.

В Solomon существуют зависимости между объектами - например, нельзя создать алерт в проекте без создания проекта. Для корректного разрешения таких зависимостей SOLO обрабатывает объекты Solomon в [заранее заданном порядке](https://a.yandex-team.ru/arc_vcs/library/python/monitoring/solo/handlers/solomon/__init__.py?rev=ee52fb08837450582dea13d2c37ccb28f834aeab#L14).


#### Особенности работы с Juggler ####

`local_id = "{host}$${service}"`

Можно использовать NoneState, поскольку в Juggler в качестве идентификатора объекта используется пара host + service.

Для работы с конфигурацией Juggler требуется указание juggler_mark - уникального строкового идентификатора, который сохраняется в списке тегов всех создаваемых контроллеров агрегатов - [подробнее про juggler mark можно прочитать тут](https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/juggler_sdk/api.py?rev=r7365779#L38), если кратко - он нужен для отслеживания объектов, созданных пользователем через API.

#### Особенности работы с Yasm ####

`local_id = "{user}$${object_id}"` - для панелей
`local_id = "{object_id}"` - для остальных объектов (алерты, шаблоны алертов и панелей)

Для работы с Yasm не требуется аутентификация.

#### Особенности работы с Monitoring ####

`local_id = "{project_id}$${object_type}$${id}"`

В Solomon есть ограничение на уникальность идентификатора - он должен быть уникален в рамках одного проекта для одного класса объектов - поэтому local_id включает в себя project_id и object_type.


### Реализация CLI ###

Для удобства пользователей библиотека также предоставляет набор вспомогательных функций для реализации простейшего варианта CLI - в случае базового сценария использования SOLO настоятельно рекомендуется воспользоваться ими.

Внутри произойдет поиск необходимых токенов, элементарная настройка системы логирования, создание контроллеров и обработка переданных объектов.

```python
from library.python.monitoring.solo.helpers.cli import build_basic_cli_v2

if __name__ == "__main__":
    cli = build_basic_cli_v2(
        objects_registry=[s_alert, j_check],  # just a list with juggler and solomon objects
        juggler_mark="adfox_production_core_juggler",  # can be skipped if you don't plan to create juggler checks
        tokens_secret_id="sec-01dg08dzh16dvg4m0176mfm7vy"  # can be skipped if you plan to provide tokens via env vars or library.python.oauth
    )
    cli()
```

Если же вам хочется расширить возможности CLI, например, хочется выбирать набор создаваемых объектов - можно воспользоваться функцией `solo.helpers.cli.basic_run_v2`, используемой внутри `solo.helpers.cli.build_basic_cli_v2` - [пример подобного сценария можно найти тут](https://a.yandex-team.ru/arc/trunk/arcadia/yabs/solo_yabs/creator/__main__.py).

### Примеры ###

Для начинающих пользователей SOLO мы подготовили шаблонный проект, использующий базовый вариант CLI-интерфейса и содержащий простые примеры создания объектов Solomon и Juggler - [найти его можно тут](https://a.yandex-team.ru/arc_vcs/library/python/monitoring/solo/example/example_project). Проект состоит из двух частей - PY3_PROGRAM `creator`, использующей `build_basic_cli_v2` для создания CLI, и PY3_LIBRARY `registry` для хранения конфигурации объектов. В выборе структуры собственного проекта начинающим пользователям предлагается ориентироваться на данный пример - подобная структура хоть и может показаться избыточной вначале, но она избавит вас от дополнительной работы по группировки объектов в будущем.

---

## Эксплуатация ##

### Описание объектов ###

Следует стараться использовать существующие объекты при создании новых, увеличивая их связность. Например, если мы создаем алерт в некотором уже созданном через SOLO проекте `project`, то следует в `project_id` конфигурации алерта указать `project.name`. Подобный подход позволит избежать ошибок, а также упростит массовые изменения объектов в случае глобальных рефакторингов.

Аналогичное правило работает и для метрик - в целом при наличии плоского дерева лейблов рекомендуется описывать метрики отдельно и переиспользовать их при описании других объектов - алертов и графиков. К сожалению, такой подход не реализуем в случае развесистого дерева лейблов - в таком случае можно реализовать собственный Builder для метрик, сохранив общие детали конфигурации метрик в нем.

### Релизные процессы ###

Многим пользователям SOLO может быть достаточно хранения кода в репозитории и локального запуска SOLO при необходимости внести изменения в конфигурацию. Для более продвинутых кейсов - например, с генерацией конфигурации на основании данных из внешних источников - рекомендуется использовать NewCI + шедулер в Sandbox, [хороший пример можно найти тут](https://a.yandex-team.ru/projects/bs-infra/ci/releases/timeline?dir=yabs%2Fsolo_yabs&id=deploy-layer-release). Подробнее про предлагаемую схему [можно прочитать тут](https://lazuka23.at.yandex-team.ru/17).

---

### Контакты ###

* Ответственные за разработку: @lazuka23, @lshestov, @m-lyubimov
* [Чат поддержки](https://t.me/joinchat/TvSc1SmLXfhk4fIu)

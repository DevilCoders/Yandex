## Генератор дашбордов для Grafana

### Reference

#### Общие замечания

Выбор формата спецификации обусловлен:
- функциональными возможностями Grafana и плагина Solomon;
- задачей описания дашбордов и графиков наиболее простыми конструкциями.

В качестве базы выбран язык разметки yaml, предоставляющий широкие возможности при компактной записи.
Так, yaml позволяет записывать строки без кавычек и апострофов, если строка не содержит спец. символов:
```yaml
string1: value1
string2: 'value2'
string3: "value3"
map1: { string4 : value4 }
```

Некоторые поля спецификации принимают несколько вариантов `типов` значений, чтобы упростить запись. Например:
- `templates: { name: template1 }` (объект) vs `templates: [{ name: template1 }, { name: template2 }]` (массив);
- `select: { function_call_with_params_as_list: [], function_call_with_params_as_string: params_string }`.

Большое количество параметров имеет значения по умолчанию, а наиболее часто используемые сценарии описываются типовыми шаблонами.
Вместе с тем, присутствует возможность гибкой настройки и дальнейшей кастомизации генератора.

Материалы для дополнительного ознакомления:
- [Язык аналитических запросов Solomon](https://wiki.yandex-team.ru/solomon/userguide/el/) (для описания узлов `queries` и `select` в графиках);
- [Описание JSON-модели дашборда в Grafana](http://docs.grafana.org/reference/dashboard/) (результат работы генератора).

#### Параметризованные включения вложенных спецфикаций

С помощью директивы `!include sub.yaml [param1 [...paramN]]` можно включать вложенные спецификации.
Поддерживаются параметры с пробелами, для этого используется 2 типа кавычек (`'` и `"`),
внутри каждого из которых можно использовать другой тип.
Включение происходит на уровне строк/текста, а не yaml.
Путь к файлу `sub.yaml` указывается относительно родительского файла.
В тексте `sub.yaml`, параметры адресуются как `@1` ... `@N`.
Каждая строка включаемого файла получает тот же отступ, что имел исходный `!include`.
Файл `sub.yaml` может иметь вложенные включения, которые также можно параметризровать.
Доступен
[пример](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/test/resources/dashboard/testInclude) с
[тестом](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/test/java/yandex/cloud/dashboard/util/YamlTest.java).

#### Основные параметры

```yaml
folderId: # 0 – (опциональный, по умолчанию null == 0 == General) папка дашборда
id: # 79682 – (опциональный) уникальный числовой идентификатор дашборда, генерируемый Grafana
uid: # XlF7fo_iz – уникальный строковый идентификатор дашборда
time: # см. ниже - (опциональный) указывает time range дашборда, по умолчанию "3h" - последние 3 часа
refresh: # 10m – (опциональная строка) интервал автоматического обновления содержимого
title: # Demo dashboard – (строка) заголовок

variables: # См. раздел "Переменные" (ссылка ниже)

pointerSharing: # none, line (default), tooltip – режим шаринга указателя между графиками дашборда
links: # см. раздел "Ссылки и теги" (ссылка ниже)
tags: # см. раздел "Ссылки и теги"

graphDefaults: # См. раздел "Иерархия" (ссылка ниже)
queryDefaults: # См. раздел "Иерархия"

# Поддерживаются дашборды, состоящие либо только из рядов, либо только из панелей (графиков),
# но (для простоты поддержки) не одновременно, несмотря на то, что Grafana это поддерживает.
rows: # См. разделы "Группировка графиков с помощью рядов" и "Графики"
panels: # В том же формате, что и содержимое panels у элементов узла "rows"
```
См. разделы:
- [переменные](#переменные);
- [ссылки и теги](#ссылки-и-теги);
- [иерархия](#иерархия);
- [группировка графиков с помощью рядов](#группировка-графиков-с-помощью-рядов);
- [графики](#графики).

Параметр `time`: поддерживает
[относительные временные диапазоны](https://grafana.com/docs/grafana/latest/dashboards/time-range-controls/#time-units-and-relative-ranges)
в полном `time: { from: <from>, to: <to> }` или сокращённом `time: <from>` виде.
Здесь `<from>`  и `<to>` указывают смещение (`5m`, `1d`) и/или выравнивание (`/d`, `/w`) относительно `now`.
Примеры (и [ещё](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/test/java/yandex/cloud/dashboard/model/spec/dashboard/TimeSpecTest.java?rev=6978936#L15)):
`5m` - последние 5 минут, `{ from: '2d/d', to: '2d/d }` - позавчера, `/w` - период от начала текущей недели.

Обязательны только параметры `id`, `uid` и `title`.

##### Иерархия

Спецификацию можно условно разделить на 3 основные сущности:
- дашборд;
- ряд;
- панель: график, перечень ссылок на дашборды, placeholder и т.д.
  Подробнее см. в разделе "[Другие типы панелей](#другие-типы-панелей)".

Параметры дашборда – это top-level-свойства yaml-файла.
Панели задаются либо непосредственно в узле `panels` дашборда:

```yaml
panels:
  - { type: graph, name: Graph 1 ... }
  - ...
```
либо, если дашборд разделён на ряды, являются потомками элементов узла `rows`:

```yaml
rows:
  - title: Row 1
    panels:
      - { type: graph, name: Row 1 Graph 1 ... }
```

Некоторые параметры можно указать на уровне дашборда, затем пере-/доопределить на уровне ряда и, затем, графика.
Они рассмотрены ниже.

###### Параметры графика

Параметры по умолчанию определяются для дашборда и ряда:
```yaml
graphDefaults:
  datasource: Solomon
  width: 8 # 1/3 ширины (полная ширина = 24 ед.)
  height: 6 # 180px (каждая единица высоты соответствует 30px)
```

На уровне графика данное поле называется `params`:
```yaml
panels:
  - type: graph
    params: { width: 4, ... }
```

Поле `datasource` определяет инсталляцию Solomon (например, `Solomon` или `Solomon Cloud` –
с перечнем можно ознакомиться, например, в процессе создания нового графика в UI Grafana).
В перспективе, оно также может указывать на другие типы datasource.

###### Параметры запроса в графике

Каждый график содержит один или несколько запросов, результатом которых являются "линии" – содержимое графика.
Параметры запроса содержат метки (`labels`), часть из которых можно определить на уровне дашборда, ряда или графика
(`queryDefaults`), а часть – на уровне queries в графике (поле `params`).

```yaml
queryDefaults: { labels: ... }
...
panels:
  - type: graph
    queries:
      - params: { labels: ... }
```

ⓘ Метки можно не только доопределять, но и "удалять". Например, если на уровне всего дашборда определена метка
`host`, которая не нужна в одном из рядов, можно убрать её из списка меток с помощью значения `-`: `host=-`.

Поскольку описание меток встречается часто, для удобства, поддерживается несколько форматов указания меток. ⚠ Формат
записи `key=!value` – deprecated. Просьба использовать формат: `key!=value`.

```yaml
  # URL format (при копировании из URL к Solomon Auto Graph или из вызова load() в Grafana panel json)
  labels: 'project=yandexcloud&cluster=cloud_preprod_api-adapter&service=api_adapter'
```

```yaml
  # Формат, получамый при нажании кнопки копирования меток [Copy] на странице графика Solomon
  labels: 'project="yandexcloud", cluster="cloud_preprod_api-adapter", service="api_adapter"'
```
```yaml
  # Можно без кавычек/пробелов
  labels: 'project=yandexcloud, cluster=cloud_preprod_api-adapter, service=api_adapter'
```
```yaml
  labels: project=yandexcloud,cluster=cloud_preprod_api-adapter,service=api_adapter
```

В `queryDefaults` и `<graph>.queries[].params`, помимо `labels`, можно указать:
- `defaultTimeWindow` (см. ниже) – переопределяет значение по умолчанию
  `default` (== `$__interval`) для агрегации по времени (`groupByTime`);
- `dropNan: true` – при этом во все запросы графика будет добавлена функция `drop_nan`.

#### Группировка графиков с помощью рядов

С помощью рядов можно удобно сгруппировать графики.
Ряды имеют название, могут схлопываться и дают возможность пере-/доопределить общие параметры графиков ряда.
```yaml
rows:
  - title: Some row
    graphDefaults: { width: 12 }
    # collapsed: false
    panels:
      - type: graph
        params: { width: 8, height: 6 } # перегружает width, добавляет height
        ...
```

#### Графики

Определим простой график RPS. Предположим, метки `project`, `cluster` и `service` уже определены на уровне дашборда.
```yaml
panels:
  - type: graph
    title: RPS graph
    queries:
      - params: { labels: 'host=cluster, sensor=grpc_requests, meter_type=count, app=cloud-api-adapter_server, method=all' }
        groupByTime: { max: 'default' }
        select: { non_negative_derivative: [], alias: 'rps' }
```

- `title` и `description` – заголовок (обязательный) и описание (опциональное), соответственно;

- `params` - набор labels, описывающих timeserie(s);

- `groupByTime` – разбивает каждый timeserie на непересекающиеся интервалы времени и заменяет значения интервала на результат применения указанной
[агрегирующей функции](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/GroupByTimeSpec.java?rev=6517068#L26).

  Примеры значений:
    - `30s`, `1m` (30 секунд и 1 минута соответственно);
    - `$__interval` – значение интервала автоматически выбирается Grafana, в зависимости от текущего масштаба;
    - `default` – использовать специальное значение, по умолчанию равное `$__interval`, которое можно переопределить
      с помощью `defaultTimeWindow: <value>` в блоках `queryDefaults` и `<graph>.queries[].params`.
      Значение `default` используется генератором дашбордов во всех стандартных шаблонах (например, `errors`).

  Выполняется до функций раздела `select` (см. ниже).

  Используется, в частности, для downsampling; чтобы снизить нагрузку на datasource, `groupByTime` участвует в запросе
  всегда, по умолчанию – с параметрами `($__interval, 'avg')`;

- `select` – упорядоченный набор функций, применяемый к timeserie(s). Порядок – слева направо.
  В данный момент, одна функция может участвовать в выражении только один раз (по необходимости, можно поддержать множественные вызовы).

  Перечень доступных функций:
  - поддержанных в генераторе –
    [здесь](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/result/panel/FunctionDescriptor.java#L33);
  - полный список с описанием –
    [здесь](https://wiki.yandex-team.ru/solomon/userguide/el/).

  ⚠ Количество параметров любой функции зависит от способа её
  [вызова](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/result/panel/FunctionDescriptor.java#L33).
  Для `normalCall`, их число должно быть на единицу меньше, чем в соответствующем списке (см. ссылку выше).
  Так, в спецификации для функции `"alias"`, описанной дескриптором `normalCall(CHAINED, QUOTED)`,
  следует указать один параметр: `alias: [rps]` (или просто: `alias: rps`).
  Оставшийся параметр (`CHAINED`) предназначен для реализации вложенных вызовов функций, являясь точкой подстановки уже вычисленного подвыражения.

  Результат будет таким:
  ```json
  {
    "target": "alias( fn1(fn2(...)), 'rps')"
  }
  ```

  Параметры могут быть указаны списком строк: `[p1, p2, p3]` или строкой: `'p1, p2, p3'`. Пустая строка == `[]`.
  ⚠ Другие типы не поддерживаются, поэтому числовые параметры следует указывать строками.

  Функции могут иметь сокращённые имена. Например, вместо `histogram_percentile` можно использовать `hist`.

  Попробовать применение функций в интерактивном режиме можно в UI создания графика в Grafana.

ⓘ Если некоторое выражение для `target` невыразимо с помощью доступных инструментов,
[можно указать его непосредственно в JSON](https://wiki.yandex-team.ru/solomon/grafana/#expression)
с помощью синтаксиса (пример):
```yaml
    queries:
      - expr: "top(10, 'max', group_by_time($__interval, 'avg', {...}))"
        #params: { labels: '...' } # если необходимо для иерархического вычисления @labels
```
Для подстановки текущих меток селектора, внутри `{...}` можно использовать `@labels` (или `@{labels}`). Примеры:
- явное указание меток селектора: `{label='value1', 'quoted'='value2'}`;
- подстановка текущих меток (включая указанные в `params` рядом с `expr`): `{@labels}`.
  Так, в [данном файле](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/test/resources/dashboard/demoExpr.yaml)
  задаются два графика с идентичными выражениями в `target`;
- комбинированный подход: `{@labels, additionalLabel='value'}`.

##### Отображение линий графика

График содержит один или несколько запросов. Каждый из них может задать одну или
(с помощью `label=*`, `label=case1|...|caseN` и т.д.) несколько линий.

Каждая из них получает некоторе имя. Выбор имени происходит приблизительно так (в порядке убывания приоритета):
- имя задаёт well-known функция. Так, `alias(rps)` даст линии название `rps`, а `percentile_group_lines(90)` – `p90.000000`;
- если таких функций в запросе несколько, приоритет имеет последняя из них;
- если в запросе присуствует функция-агрегат (например, `group_lines(sum)`), то название соответсвует названию query.
  Генератор даёт запросам названия `Q1`, `Q2` и т.д.;
- если метки запроса определяют единственный сенсор, то берётся название сенсора (метка `sensor`);
- если метки определяют несколько сенсоров, то показывается декартово произведение переменных значений.

Увидеть получившиеся имена получившихся линий можно, выгрузив дашборд в Grafana.

Далее, отображение каждой каждой линии графика можно кастомизировать по её имени:
- цвет;
- Y-ось (на графике можно отобразить 2 Y-оси, с независимыми шкалами).
- stack: true|false
```yaml
    draw: [{ alias: '4xx', color: '#0a437c', at: right }, { alias: '5xx', color: '#bf1b00', at: left, stack: true }]
```

ⓘ NB
- `at: left` – значение по умолчанию.
- цвет можно указать в сокращённом формате: `#rgb` – то же, что `#rrggbb`, либо в виде названий (color: red).

##### Настройки отображения и легенда

Следующие настройки графика влияют на отображение (все параметры опциональны):
- легенды и tooltips:
  ```yaml
  display:
    legend: true # варианты отображения легенды. Возможные значения: bottom, right, false, true (default) (true эквивалентно bottom)
    decimals: 2 # количество знаков после запятой в легенде и tooltip
    empty: false # показывать ли в легенде серии только с nulls/zeroes
    sort: false # сортировать ли серии в tooltip: none | 0 | off | no | false (default); increasing | 1 | on | yes | true; decreasing | 2
    stack: false # отображать ли серии на графике в виде стека
    lineModes: bars # одно значение / список различных значений из варриантов: bars, lines (default), points
    lineWidth: 1 # толщина линий
    fill: 1 # степень непрозрачности графика, 0 - 10
    nulls: keep # keep = не менять, zero = заменять нулями, connected = не-null-значения интерполируются без учёта nulls
  ```
- оси (или осей, если на графике их две):
   ```yaml
   yAxes:
   - decimals: 2 # количество знаков после запятой
     format: decbytes # один из форматов, поддерживвемых Grafana: none, short, ms, s, decbytes и т.д.
     label: rps #  название оси
     logBase: 1 # настройка логарифмической шкалы
     max: 10 # максимальное значение
     min: 0 # минимальное значение
   ```

#### Шаблоны

Отображение RPS, как на графике выше, является одной из типовых задач. Чтобы сократить декларацию типовых графиков,
вводится понятие `шаблон (template)`. Шаблон является способом трансформации спецификации графика, изменяя и
добавляя специфичные для текущей задачи атрибуты. Для примера, возьмём шаблон RPS: по умолчанию, он принимает
монотонно растущий счётчик – сенсор типа `counter`, назначает сглаживание и применяет (для этого типа сенсора) производную,
а также кастомизирует отображение Y-оси (число знаков после запятой, unit оси, название "rps", минимальное значение Y=0).
Определение графика сокращается до:
```yaml
  - type: graph
    templates: { name: rps }
    title: RPS graph (using template)
    queries:
      - params: { labels: 'host="cluster", sensor="grpc_requests", meter_type="count", app="cloud-api-adapter_server", method="all"' }
```
и после применения шаблона спецификация примет вид, напоминающий приведенный выше в разделе "Графики" пример:

```yaml
  - type: graph
    title: RPS graph (using template)
    yAxes: [{ decimals: 1, format: short, label: rps, min: 0 }]
    queries:
    - params: { labels: 'app="cloud-api-adapter_server", host="cluster", meter_type="count", method="all", sensor="grpc_requests"' }
      groupByTime: { max: '30s' }
      select:
        group_lines: "sum"
        nn_deriv: []
        alias: rps
```

Помимо `name`, шаблоны могут иметь параметры, меняющие их поведение.

Детальное описание имеющихся шаблонов, их параметров и поведения, вместе с примерами использования – находится на
[этой странице](TEMPLATES.md).

Примеры фактического использования шаблонов доступны в
[demo-спецификации](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/test/resources/dashboard/demo.yaml).

Актуальный перечень всех шаблонов можно найти в
[этой папке](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template).
Чтобы добавить новый шаблон, в данный момент, требуется создать здесь файл с именем `<Name>Template.java` и
прописать его в свойствах одного из наследников
[этого интерфейса](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/Template.java):
[GraphTemplate](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/GraphTemplate.java#L11)
или
[SsTemplate](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/SsTemplate.java#L11).
В дальнейшем, возможна поддержка динамически загружаемых шаблонов-плагинов.

⚠ При проектировании нового шаблона, следует избегать задания параметра в спецификации шаблона с его последующим копированием
в неизменном виде в спецификацию графика. Такой параметр лучше указать непосредственно в спецификации графика
(в этом случае, шаблон также будет иметь к нему доступ). Например, такой вариант:
```yaml
  templates: { name: templateThatSetsUpDisplay, stack: true } # создаст: display { stack: true, somethingElse: ... }
```
лучше заменить на:
```yaml
  templates: { name: templateThatSetsUpDisplay }
  display: { stack: true } # шаблон увидит это, смёржит и преобразует в: display { stack: true, somethingElse: ... }
```

##### Несколько шаблонов

Когда шаблон содержит логику значительного размера, его сложно кастомизировать: нужно указывать параметры шаблона,
которых со временем может стать слишком много.

Так, шаблоны `rps` и `errors` содержат одинаковую функциональность, назначая линиям название (`alias`).

Ввиду сказанного выше, удобно вынести эту общую функцинальность в отдельнй шаблон, тем самым уменьшив
размер и область ответственности шаблона.

Это было поддержано, и в спецификации можно указывать несколько последовательно применяемых шаблонов:
```yaml
    templates: [{ name: template1, params1: value1, ... }, { name: template2 }]
```
В данном случае:
```yaml
    templates: [{ name: rps }, { name: alias ... }]
```

##### Вложенные шаблоны

Основной сценарий использования шаблона `rps` – агрегированный RPS, а не отдельные линии.
Это тот случай, когда следует назначить агрегированной линии на графике явный `alias` с названием линии по умолчанию: 'rps'.
Чтобы сократить декларацию в этом наиболее частом случае, и были поддержаны вложенные шаблоны.
Это значит, что любой шаблон может указать, какие подшаблоны должны быть вызваны "до" и "после" него, и с какими параметрами.
Благодаря этой возможности, для rps-шаблона,
[декларирующего вызов после себя шаблона alias](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/RpsTemplate.java?rev=6516178#L50)
в случае суммирования линий, следует вместо `templates: [{ name: rps }, { name: alias, alias: rps }]` указать просто `templates: { name: rps }`.

#### Другие типы панелей

##### Список дашбордов (Dashboard list)

Панель служит для отображения группы дашбордов (`starred`, `recent`, поиск по названию или тегам, папке).
[Модель](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/DashboardListSpec.java),
[примеры](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/resources/dashboard/main.yaml).

```yaml
panels:
  - type: dashlist
    title: Personal
    # description: ...
    headings: true
    starred: true
    recent: true
    limit: 5
    # ... params { width, height }, search { query, tags, folderId }
```

##### Пустая панель (Placeholder)

Добавляет пустую панель в текущую строку.
Это бывает нужно для того, чтобы последующие панели отображались на новой строке.
[Модель](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/PlaceholderSpec.java).

```yaml
panels:
  - type: placeholder
    # description: ...
    # ... params { width, height }
```

##### Светофор (Singlestat)

Панель служит для отображения одного значения, сопровождая его визуализаций для быстрого восприятия.
Эту панель можно использовать, например, для отображения текущих метрик, таких, как свободный объём памяти.
[Модель](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/SinglestatSpec.java).

Пример:
```yaml
panels:
  - type: singlestat
    title: 'Adapter errors $cluster (total)'
    # description: ...
    templates: { name: trafficlights, lightsCount: 4, thresholds: [20, 250, 500] }
    gauge: { maxValue: 500, show: true, thresholdLabels: true }
    query:
      params: { labels: 'sensor=grpc_statuses, status=@5xx' }
      groupByTime: { 'max': 'default' }
      select: { diff: [], drop_below: '0',  group_lines: 'sum' }
    value: { valueFunction: 'total' }
```

###### Данные

Панель содержит запрос [query](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/QuerySpec.java),
схожий с таковым у графиков, к результатам которого будет применена агрегационная функция `value:valueFunction`.

Агрегационные функции:
* `min` - наименьшее значение в серии;
* `max` - самое большое значение в серии;
* `avg` - среднее значение всех ненулевых значений в ряду;
* `current` - последнее значение в ряду; Если серия заканчивается на null, будет использоваться предыдущее значение;
* `total` - сумма всех ненулевых значений в ряду;
* `first` - первое значение в серии;
* `delta` - общее инкрементное увеличение (счетчика) в серии. Предпринимается попытка учесть сбросы счетчиков, но это будет
 точно только для метрик одного экземпляра. Используется для отображения общего увеличения счетчика во временных рядах;
* `diff` - разница между `current` (последнее значение) и `first`;
* `range` - разница между `min` и `max`;

###### Настройка отображения

Настройка цветов отображения – поле [coloring](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/SsColoringSpec.java).
Настройка отображения результирующего значения находится в поле [value](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/SsValueSpec.java).
Настройка отображения шкалы – поле [gauge](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/SsGaugeSpec.java).
Шкала даёт четкое представление о том, насколько высоко агрегированное значение в заданых рамках.
Шкала – это способ увидеть, если значение близко к пороговым значениям. Датчик использует цвета,
заданные в параметрах цвета.
Настройка отображение `спарклайна` на фоне – поле [sparkline](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/SsSparklineSpec.java).
`Спарклайн` – отображение, позволяющее увидеть исторические данные, обеспечивая ценный контекст с первого взгляда.
Спарклайны действуют иначе, чем традиционные графики, и не включают оси `x` и `y`, координаты, легенду и возможность
взаимодействия с графиком.

###### Шаблоны

[TrafficLightsTemplate](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/TrafficLightsTemplate.java)

#### Переменные

##### Статические

Позволяют избегать magic-констант, а также менять отдельные значения для нескольких графиков сразу.
Например, мы могли бы определить константы для перечня 4xx- и 5xx-статусов:
```yaml
variables:
  replacement:
    4xx: CANCELLED|INVALID_ARGUMENT|NOT_FOUND|ALREADY_EXISTS|PERMISSION_DENIED|RESOURCE_EXHAUSTED|FAILED_PRECONDITION|ABORTED|OUT_OF_RANGE|UNAUTHENTICATED
    5xx: UNKNOWN|DEADLINE_EXCEEDED|UNIMPLEMENTED|INTERNAL|UNAVAILABLE|DATA_LOSS
```
И использовать их так на графике ошибок сразу нескольких grpc-сервисов с помощью синтаксиса:
```yaml
  labels: '..., status="@4xx"'
```

Теперь, если мы решим вынести `UNAVAILABLE` из `4xx` в `503`, это можно будет сделать в едином месте.

Значения переменных могут ссылаться на другие переменные.

##### UI-селекторы

Другой тип переменных использует встроенную возможность Grafana отображать dropdown list на UI, динамически меняя
выбранное значение переменной.
```yaml
variables:
  ui:
    cluster: { values: [prod, preprod] }
```

Значение, указанное в списке первым (`prod`), будет выбрано по умолчанию.
Поддерживаются некоторые другие настройки, включая названия опций, отличные от своих значений
(см. ниже в [секции](#повтор-клонирование-графиков-светофоров-и-рядов) о повторе графиков и рядов).

Использование:
```yaml
  labels: '..., cluster="$cluster"'
```

Альтернативный ситнаксис: `${cluster}`, `[[cluster]]`.

##### UI Query-селекторы

Это UI-селекторы специального вида, список значений которых представляет собой не набор заранее заданных констант,
а перечень значений некоторой метки в запросе вида `{label1=value1 ... labelN=valueN}`.

Так, удобно получать значение метки `host` в зависимости от окружения: `{project=..., cluster=$env, service=..., host=*}`.

Синтаксис (пример):
```yaml
variables:
  uiQuery:
    host:
      datasource: 'Solomon Cloud' # можно задать в graphDefaults.datasource
      labels: 'project=instance-group, cluster=${cluster}, service=core, host=*' # происходит merge из queryDefaults.labels
      inheritLabels: true [= true] # в случае false, отключает наследование меток из queryDefaults.labels
      label: host [= uiQuery var name] # опциональна, по умолчанию равна названию uiQuery-переменной
      regex: <PCRE> # опциональное выражение для фильтрации полученных значений (частичный match; для полного - "^...$")
                    # результат можно ограничить, указав его в (скобках): "ignoredPrefix(capturedText)ignoredSuffix"
      multi: true # [= false]
      hidden: false # [= false]
```
ⓘ Допускается получать даже метки шарда, например, `cluster` по `project`.
ⓘ Будут выбраны все значения метки, включая даже отсутствующие в точках, попавших на график.
⚠ Искомая метка (`label`) всегда должна попадать в запрос. Например: `label=*`.

##### Использование UI-переменных

- использование значения для подстановки в названия графиков и рядов, а также query labels;
- для multi-value-переменных: показ выбранных линий на одном графике с помощью
  [объединения значений переменной через ${var:pipe}](https://grafana.com/docs/grafana/latest/variables/advanced-variable-format-options/#pipe);
- повтор графиков и рядов (см. далее).

##### Повтор (клонирование) графиков, светофоров и рядов

С помощью единого параметризованного шаблона, можно сформировать несколько однотипных графиков.
То же доступно и для светофоров, и для рядов.

К примеру, мы хотим отобразить отдельный график для каждого хоста кластера.

###### Статический способ

В этом случае, генератор формирует каждый график статически, не используя каких-либо функций системы отображения.

Для этого задаётся переменная типа `repeat`, содержащая список значений:
```yaml
variables:
  repeat:
    preprod_host: { values: [ api-adapter-sas1, api-adapter-vla1, api-adapter-myt1 ] }
```
и используется так:
```yaml
panels:
  - type: graph
    repeat: preprod_host
    title: 'Repeating (cloned) graph, current host = @preprod_host'
    queries:
      - params: { labels: 'host="@preprod_host" ...' }
```

Рекомендуется располагать шаблон графика в начале строки (внутри ряда), хотя для статического способа это и не обязательно.
Полученные графики будут размещены на дашборде или в ряде последовательно (вправо-вниз) с автоматическим переносом.

В перечне значений можно указать специальное значение `--`.
Если оно встречается в метках, вместо графика будет добавлена панель типа `placeholder`.

Если переменную задать в расширенном виде:
```yaml
    preprod_host:
      values: [ api-adapter-sas1, ...]
      titles: [ 'Host sas1', ...]
      variables:
        var1: ['var1-value for sas1', ...]
        var2: ['var2-value for sas1', ...]
```
то можно ссылаться как на значения (`@preprod_host` или `@{preprod_host}`),
так и на соответствующие заголовки (`@preprod_host:title` или `@{preprod_host:title}`)
и переменные (`@preprod_host:var1` или `@{preprod_host:var1}`).

###### Динамический способ

Данный способ требует использования UI-переменных
(см. [раздел выше](#использование-ui-переменных)).
Если задать UI-переменную в расширенном виде
(все поля опциональны; требуется указать лишь `values` и/или `conductor`):
```yaml
    preprod_host:
      multi: true # Допускается несколько значений одноременно (выбор с помощью checkbox) + значение "All" (по умолчанию)
      hidden: true # Не отображать на UI dropdown list для данной переменной
      values: [ api-adapter-sas1, ... ]
      titles: [ 'Host sas1', ... ]
```
то, добавив на график директиву:
```yaml
    uiRepeat: preprod_host
```
получим повтор графика средствами Grafana, причём состав отображаемых графиков можно менять с помощью UI dropdown list.

Ссылка на заголовок переменной (`title`) имеет такой же вид, что и ссылка на значение – `$preprod_host`.
При этом, Grafana по контексту (переменная вставляется в `title` или в `labels`?) определяет, что именно нужно подставить.

⚠ Для данного способа, важно располагать шаблон повторяемого графика в начале строки (но не обязательно в начале ряда).

###### Интеграция с conductor

В переменных (статических и UI) можно указать параметр `conductor: <group>`.
В результате получится список хостов данной группы.
Такая запись является сокращённой от `conductor: { group: <group>, mode: host, fqdn: true }`.

Если указать: `conductor: { group: <group>, mode: tree }`, то получится список, состоящий из групп,
подгрупп и хостов данной группы, со значениями вида `host1|...|hostN` (селектор всех хостов данной опции).
Такой список полезен для организации drilldown (см. [далее](#drilldown)).

Если одновременно с `conductor` указаны также `values` (возможно, с `titles`), то они будут добавлены в начало списка.

Параметр `fqdn` соответствует настройке `Use FQDN` кластера `Solomon` и указывает,
как `Solomon Agent` передаёт имя хоста – в коротком либо полном виде.

###### Повтор светофоров (singlestat)

Повтор светофора аналогичен повтору графика. С той лишь разницей, что для светофора доступен только статический способ повтора.
(При необходимости можно легко добавить в генератор поддержку динамического повтора светофоров.)

Статический способ:

```yaml
variables:
  repeat:
    iamService:
      values: [ access-service, token-service, iam-control-plane ]
      titles: ['Access Service', 'Token Service', 'IAM Control Plane' ]
      variables:
        link_name: [ 'http://url1', 'http://url2', 'http://url3' ]

panels:
  - type: singlestat
    title: '@iamService:title'
    value: { valueFunction: total }
    links:
      - title: External Link
        url: '@iamService:link_name'
    query:
      select: { diff: [], drop_below: '0', group_lines: [ 'sum' ] }
      params: { labels: 'project=@iamService, sensor=grpc_statuses, status=@5xx503' }
```

###### Повтор рядов

Ряд повторяется целиком со всеми составляющими его графиками.
Зачастую, группировка с повтором ряда более наглядна, чем несколько рядов, каждый из который содержит повторяющийся график.

Повтор ряда аналогичен повтору графика.

Для повтора ряда доступен как динамический, так и статический способ:

Статический способ:
```yaml
variables:
  repeat:
    client_app:
      values: ['auth', 'iam', 'resource-manager']
      titles: ['Auth Client', 'IAM Client', 'Resource Manager Client']
rows:
  - title: '@client_app:title'
    repeat: client_app
```

Динамический способ:
```yaml
rows:
  - title: Some row
    uiRepeat: preprod_host
```

###### Drilldown

Иногда для исследования причин проблемы нужно иметь возможность детализации агрегированных данных на графиках.
Это возможно с инструментом `drilldown`, который в общем случае работает так: часть данных (например, ряд графиков)
с дашборрда выносятся на отдельный дашборд и дублируются по одной или нескольким переменным с помощью одного из способов выше.

Сейчас реализовано дублирование только ряда графиков, повтор по UI-переменной, переменная повтора только одна,
однако можно использовать дополнительные UI-переменнные для фильтрации.

Реальный пример (и [ещё один](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/resources/dashboard/duty/duty.yaml?rev=5785058#L102)):
```yaml
uid: ycpDuty
...
rows:
  panels:
  - ...
    drilldowns:
      - subUid: dd_api_adapter_host # альтернативно, можно задать полный uid: ycpDuty_dd_api_adapter_host
        tags: [ 'ycp', 'ycp-adapter', 'ycp-duty' ]
        ui:
          cluster: cluster
          host:
            multi: true
            values: [ '!cluster', '*-myt*', '*-sas*', '*-vla*', '*-myt1', '*-sas1',  '*-vla1', '*-myt2', '*-sas2', '*-vla2' ]
            titles: [ 'Cluster', 'DC myt', 'DC sas', 'DC vla', 'Host myt1', 'Host sas1',  'Host vla1', 'Host myt2', 'Host sas2', 'Host vla2' ]
          method:
            values: [ 'all', 'compute.*', 'iam.*', 'loadbalancer.*', 'mdb.*', 'resourcemanager.*', 'vpc.*' ]
            titles: [ 'All services', 'Compute', 'IAM', 'Load Balancer', 'MDB', 'Resource Manager', 'VPC' ]
        uiRepeat: host
        labels: 'host=$host, method=$method'
```

Атрибуты используются для создания drilldown-дашборда:
- `uid` задаёт абсолютное значение uid, который можно также задать через `subUid` в виде: uid = `<parent dashboard uid> _ <subUid>`;
- `tags` – определяют теги + дополнительно добавляется тег `<parent dashboard uid>.replace('_', '-') + "-auto-drilldown"`;
- `ui` (`uiQuery`) – список ui (uiQuery)-переменных. Каждое значение:
  - либо строка – ссылка на ui (uiQuery)-переменную parent dashboard'а,
  - либо задаёт спецификацию новой ui (uiQuery)-переменной;
- `uiRepeat` – переменная, используемая для повтора ряда (должна присутствовать в списке `ui` или `uiQuery`);
- `labels` – набора меток, который переопределяет имеющиеся метки на графиках.

ⓘ Также можно задать атрибут `title`, переопределив значение по умолчанию:
`<parent dashboard title> - <row title> - <название переменной uiRepeat>`.
Это может быть также полезно, если для одного ряда заданы несколько drilldown'ов с одним и тем же `uiRepeat`.

Ограничения:
- переопределённые метки добавляются на все графики, даже там, где это не требуется;
- иногда недостаточно переопределить метки, но также нужно поменять набор функций в `query` (например, убрать `group_lines`, `alias`).

Набор из нескольких дрилдаунов создается при повторе ряда со статической переменной:
для каждого значения переменной создается свой дрилдаун с уникальным id.
Реальный [пример](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/resources/dashboard/container-registry/cr-drilldown.yaml?rev=5806738#L126):
```yaml
variables:
  repeat:
    client_app:
      values: ['auth', 'iam', 'resource-manager']
      titles: ['Auth Client', 'IAM Client', 'Resource Manager Client']
      variables:
         subUid: ["dd-cr-auth-cs", "dd-cr-iam-cs", "dd-cr-resource-manager-cs"]
...
rows:
  - title: '@client_app:title'
    repeat: client_app
    queryDefaults: { labels: 'service=${service}, app=@{client_app}_client, method=*' }
    graphDefaults: { width: 8 }
    drilldowns:
      - subUid: '@client_app:subUid'
        tags: [ 'ycp', 'ycp-cr', 'ycp-duty' ]
        ui:
          cluster: cluster
          service: service
          host:
            multi: true
            values: [ '*-myt*', '*-sas*', '*-vla*', '*-myt-1', '*-sas-1',  '*-vla-1', '*-myt-2', '*-sas-2', '*-vla-2' ]
            titles: [ 'DC myt', 'DC sas', 'DC vla', 'Host myt1', 'Host sas1',  'Host vla1', 'Host myt2', 'Host sas2', 'Host vla2' ]
        uiRepeat: host
        labels: 'host=$host'
```

Зачастую исходный график может использовать агрегаты Solomon. В этом случае, нужно учесть, что:
- может не оказаться нужного агрегата. Например, при раскрытии `host=cluster, method=all` по `host`, обязательно должен быть
  агрегат по `method` – либо значение `method` тоже должно быть переопределено в спецификации drilldown'а;
- при замене нужно следить, чтобы выбранные сенсоры не включали в себя лишние агрегаты. Например, если бы в примере выше
  мы хотели добавить фильтрацию по `method=$method`, то при `method=*` вместо `method!=all`,
  наличие агрегата `method=all` удвоило бы нужное значение;
- можно "упереться" в ограничение на количество выбранных сенсоров (по умолчанию, 10 000).

###### Замечания

Представляется удобной возможность получения значения одной переменной в зависимости от выбранного значения другой (`map`).
Статические переменные не позволяют добиться такого в принципе, однако зачастую результат можно получить при помощи
динамических UI-переменных в комбинации с доступным набором возможностей:
- метка в запросах графика может содержать `glob` (`host=*`, `host=*-DC1-*`)
  и/или текущие значения single-value (`host=$host`) и multi-value (`host=${host:pipe}`) UI-переменных;
- графики и ряды могут использовать статические (`repeat: <static var>`) и динамические (`uiRepeat: <UI var>`) повторы;
- можно получать набор значений метки из Solomon по динамическому селектору через `uiQuery`
  (например, значения метки `host` по выражению `project=myProject, cluster=$env, service=myService, host=*`).

Пусть имеется дашборд, отображающий, в зависимости от значения переменной `env`, данные для окружений `preprod` и `prod`,
отличающихся хостами. Рассмотрим примеры:
- для отображения несколько линий на одном графике, достаточно указать в его запросах: `host=*`;
- если нужно отобразить 1 график на хост без разбивки хостов на подгруппы, подойдёт `uiQuery` + `uiRepeat`;
- если названия хостов обладают семантикой, её можно "захардкодить" (статически или динамически) в переменную
  безотносительно текущего окружения и подставлять в метку `host`. Например, `dc: [ values: { DC1, DC2, DC3 }`,
  `host=*-@dc-*` или `host=*-$dc-*`. Но это не очень гибко: так, если в трёх DC на `preprod` – 3 хоста, на `prod` – 6,
  и дашборд имеет 6 статически созданных графиков для `{'DC1', 'DC2', 'DC3'} x {1, 2}`,
  то на `preprod`, 3 из 6 графиков всегда будут пустыми;

Другий пример – метрики `YDB`
([спецификация](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/resources/dashboard/ydb/ydb.yaml),
[дашборд](https://grafana.yandex-team.ru/d/ycpYdb)).
Шард `Solomon` определяется, как обычно, кортежем <`project`, `cluster`, `service`>.
Каждый `service` содержит некоторое (возможно, различное) множество `database`.
Задача состоит в том, чтобы выбрав `cluster`, показать графики для некоторой `database`.
Ограничения:
- метрики каждый базы могут быть отсутствовать в некоторых сервисах,
  поэтому получить название всех баз по отдельно взятому сервису через `uiQuery` не получится;
- получить список всех баз по всем сервисам также нельзя – `Solomon` не поддерживает кроссшардовые запросы;
- "захардкодить" список баз для каждого кластера сейчас нельзя (почему – см. ниже);
- допустим также, что название `database` невыразимо через `cluster`, например:
  - `cluster` ∈ `yandexcloud_preprod_global` | `yandexcloud_prod_global`;
  - `database` ∈ `/pre-prod_global/<db>` | `/global/<db>`, где `<db>` ∈ `ycloud | dns | instance-group`.

Решение здесь заключается в указании ui-переменных со следующими "захардкоженными" значениями:
- `cluster` ∈ `yandexcloud_preprod_global` | `yandexcloud_prod_global`;
- `database` ∈ `*/dns` | `*/instancegroup` | `*/ycloud`.

Тогда:
- запрос к фактическому названию базы происходят в рамках шарда, заданного выбранным `preprod-` или `prod-`кластером;
- мы выбираем только базу (или базы), соответствущую интересующему нас pattern'у;
- графики могут обращаться к разным `service`, причём данные по базе могут отсутствовать в некоторых `service`.

В более сложных случаях, когда по значению переменной требуется получить либо кортеж из нескольких значений,
либо список, не описываемый значениями некоторой метки (пример: хосты с именами в виде id нужно разбить по ДЦ),
требуется реализация специальных query-переменных через custom datasource; сейчас это не поддерживается.

В таких ситуациях, для решения задачи можно разнести информацию на отдельные дашборды
(и использовать параметризованный `!include`), либо отображать всё вместе на одном дашборде.

##### Datasource-переменные

В случае, когда один и тот же дашборд используется для отображения данных с разных `datasource` – к примеру,
если метрики с `preprod` и `prod` уходят в разные инсталляции [Solomon](https://wiki.yandex-team.ru/solomon),
можно использовать специальные UI-переменные Grafana:
```yaml
variables:
  ds: # datasource-переменные
    solomon: # название; пример использования: graphDefaults: { datasource: '$solomon' }
      type: 'grafana-solomon-datasource' # тип datasource, можно посмотреть в UI Grafana в Settings / JSON Model
      regex: '^Solomon Cloud( Preprod)?$' # <PCRE> - опциональное выражение для фильтрации полученных значений (частичный match)
      #hidden: false
```
ⓘ Для удобства, секция `solomon` выше вынесена в
[отдельный include-файл](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/test/resources/dashboard/include/ds-solomon.yaml?rev=7445764),
подключаемый с помощью:
```yaml
variables:
  !include include/ds-solomon.yaml
  ... # другие переменные
```
⚠ Ограничения (могут быть сняты в случае необходимости):
- drilldown-дашборды копируют все `ds`-переменные родительского дашборда без изменений;
- опция `multi`, а также `ui`-повторы для рядов и графиков для `ds`-переменных (`uiRepeat: $dsVariable`) отключены.

#### Ссылки и теги

Дашборды и графики могут иметь ссылки. Ссылки бывают двух типов:
- внешние:
   ```yaml
   # уровень дашборда или графика
   links: { title: 'External resource', url: '<http://url>' }
   ```
- на другие дашборды; дашборд определяет свои теги, на которые затем можно сослаться в ссылке с тегами в качестве фильтра:
  ```yaml
  # уровень дашборда
  tags: [ 'tag1', 'tag2' ] # собственные теги
  links: # ссылки на другие дашборды
  - title: 'Drilldown' # если title указан, результаты будут выведены в виде dropdown-списка, иначе – без группировки
    tags: [ 'tag3' ] # фильтр результатов
  ```
  ```yaml
  # уровень графика
  links:
    - { title: 'Link to a specific dashboard', uid: '<dashboard uid>' }
   ```

В `url` допускается использование переменных Grafana: `$variable`.

Общие параметры:
```yaml
  vars: true # (по умолчанию) указывает, добавлять ли UI-переменные и временной интервал в url
```

Кроме того, на уровне графиков поддерживаются [dataLinks](https://grafana.com/docs/grafana/latest/features/panels/graph/#data-link):
  ```yaml
  dataLinks:
    - title: 'Go to Charts'
      targetBlank: true
      url: 'https://url/?host=${host}&timestamp=${__value.time}&serie=${__series.name}'
  ```

`DataLinks` – это ссылки, в которых можно использовать специальные [built-in переменные](https://grafana.com/docs/grafana/latest/features/panels/graph/#built-in-variables),
значения которых зависят от конкретной точки на графике.

Так, в примере `https://url/?host=${host}&timestamp=${__value.time}&serie=${__series.name}`,
built-in переменная `${__value.time}` будет порезолвлена в текущее значение точки на линии, по которой нажал пользователь,
переменная `${__series.name}` – в имя серии, а `${host}` – это просто значение ui-переменной.

Отдельно стоит описать случай для запросов с использованием группировки по нескольким меткам
(например, `group_by_labels: [sum, label1, label2]`), не переопределяющих `alias`.
Дело в том, что в таких случаях, по умолчанию, имя серии генерируется в виде `label1=value1&label2=value2`,
что бывает удобным для использования в параметрах url-запроса.
Например, можно просто написать: `http://localhost/myapp?${__series.name}`,
что будет порезолвлено в `http://localhost/myapp?label1=value1&label2=value2`.

## Генератор дашбордов для Grafana

### Использование шаблонов (templates)

Ниже приведены общие правила применения шаблонов, на примере имеющихся реализаций для наиболее частых сценариев.

#### [RPS](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/RpsTemplate.java)

![RPS Graph](https://jing.yandex-team.ru/files/ssytnik/rps.png "RPS Graph")

Шаблон предназначен для показа одной или нескольких линий частоты запросов.

Исходные данные: rps-сенсор или монотонно растущий счётчик, для которого нужно построить rps (см. параметр `sensor`).

Параметры:
* `sumLines` (опциональный) – указывает, должна ли к линиям графика применяться группировка линий (в рамках каждого query):
  * `false` – не применять группировку (например, в случае `host=*`, можно показать на одном графике несколько линий rps, по одной на хост);
  * `true` либо `[]` (по умолчанию) – применить `group_lines(sum, ...)` для получения одной линии, равное сумме других;
  * `[label1 ... labelN]` – применить `group_by_labels(..., [label1 ... labelN], sum)` для частичной группировки;
* `rate` (опциональный) – базовый юнит:
  * `rps` (по умолчанию);
  * `rpm` – результат произведения `rps` на 60;
  * `ui` – выбор осуществляется с помощью переменной `rateUnit`, добавлемой на UI автоматиечски;
* `sensor` (опциональный) – указывает тип значения сенсора (подробнее см. [ниже](#тип-значения-сенсора)):
  * `raw` – фактическое текущее значение метрики; 
  * `rate` – производная метрики, вычисляемая по `counter`;
  * `counter` (по умолчанию) – монотонно растущая величина, к которой в каждой временной точке добавляется текущее значение.  

При значении `sumLines: true`, применяется вложенный шаблон `alias`, назначающий линии (линиям – если на графике несколько `query`)
название по значению параметра `rate`: `rps`, `rpm` или (при варианте `ui`) `rate`.
 
Пример базового варианта использования шаблона:
 
```yaml
  - type: graph
    templates: { name: rps }
    title: 'API adapter rps'
    queries:
      - params: { labels: '..., host=*, name=grpc_requests, meter_type=count' }
```

Применение `templates` преобразует данную спецификацию в следующий вид:

```yaml
  - type: graph
    title: API adapter rps
    yAxes: [{ decimals: 1, format: short, label: rps, min: 0 }]
    queries:
    - params: { labels: '..., host=*, name=grpc_requests, meter_type=count' }
      groupByTime: { max: '30s' }
      select: { group_lines: sum, nn_deriv: [], alias: rps }
```

Чтобы показать на графике отдельную линию на каждый хост, достаточно сгруппировать линии по `host`:
```yaml
    templates: { name: rps, sumLines: [host] }
```
либо (поскольку в данном случае `host` – единственная метка, принимающая несколько значений):
```yaml
    templates: { name: rps, sumLines: false }
```

Иногда требуется изменить некоторые элементы спецификации, не прибегая к существенному изменению поведения шаблона
с помощью параметров. В такое случае, обычно достаточно в исходной спецификации указать эти элементы:

```yaml
    display: { stack: true } # это дополнит спецификацию (шаблон не выставляет display) и отобразит линии в виде стека
    yAxes: [{ decimals: 2 }] # переопределит значение '1', выставляемое шаблоном 'rps' по умолчанию
```

В шаблонах, как и в обычных спецификациях, можно использовать всё возможности, в том числе – указывать переменные Grafana (`$uiVariable`):
```yaml
variables:
  ui:
    cluster: { values: [prod, preprod] }

...
    title: API adapter rps ($cluster)
```

Возможно одновременное применение повторов (см. использование `@repeatVariable`) вместе с шаблонами:
```yaml
  repeat: repeatVariable
  title: '$rateUnit for @{repeatVariable:title} (at $cluster)'
  templates: { name: rps ... }
  queries:
    - params: { labels: 'some_label=@repeatVariable' }
```

[Например](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/resources/dashboard/kms/kms-api.yaml?rev=5796434#L42):

```yaml
variables:
  repeat:
    data_http_method: { values: [ decrypt, encrypt, reEncrypt, generateDataKey, unknown ] }

...
  - type: graph
    repeat: data_http_method
    templates: { name: rps, rate: rps }
    title: 'HTTP @data_http_method rps ($cluster)'
    queries:
      - params: { labels: 'name=http_requests, meter_type=count, method=@data_http_method, app=*http_server' }
```

(В данном случае, значения `data_http_method { titles: ... }` не указаны, поэтому в `title` можно
вместо `@{data_http_method:title}` использовать просто `@data_http_method` – как и в `queries[].params.labels`).


#### [Errors](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/ErrorsTemplate.java)

![Errors Graph](https://jing.yandex-team.ru/files/ssytnik/errors.png "Errors Graph")

Шаблон предназначен для показа одной или нескольких линий количества событий, как правило – ошибок запросов.

Исходные данные: количество событий, их rate или монотонно растущий счётчик, к которму нужно применять diff (см. параметр `sensor`).
 
Параметры:
* `sumLines` (опциональный) – `false | true | [labels]`; подробнее см. [шаблон RPS](#rps);
* `sensor` (опциональный) – указывает тип значения сенсора: `raw | rate | counter`; подробнее см. [ниже](#тип-значения-сенсора).

В случае `sumLines: true` (по умолчанию), когда каждый `query` графика представляет собой ровно одну линию, 
этот шаблон имеет специальное поведение: он вызывает вложенный шаблон `alias`
[без указания параметра alias](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/AliasTemplate.java?rev=5821819#L28).
Это приводит к тому, что название каждой линии берётся из обязательного в данном случае атрибута графика `draw` – списка
[DrawSpec](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/DrawSpec.java)
(все элементы опциональны, когда контекст использования это позволяет):
  * `alias` – название линии;
  * `color` – цвет (примеры: `green`, `#b20`, `#123456`);
  * `at` - положение линии: `left` (по умолчанию) или `right`;
  * `stack` – отображение в виде стека (`true` или `false`).
  
причём количество элементов списков `queries` и `draw` должно совпадать.
Такое поведение обусловлено тем, что на графике ошибок чаще всего присутствуют несколько определённых линий –
например, `5xx`, `503` и `4xx`, которым нужно дать определённое название, "покрасить" в определённый цвет и,
если значения могут отличаться на порядки, расположить на разных осях, имеющих разные шкалы.
Так, в примере выше, количество `4xx` зачастую существенно превышает количество `5xx`.

Пример графика:
```yaml
  - type: graph
    templates: { name: errors }
    title: 'Adapter errors ($cluster)'
    queries:
      - params: { labels: 'name=grpc_statuses, status=@4xx' }
      - params: { labels: 'name=grpc_statuses, status=@503' }
      - params: { labels: 'name=grpc_statuses, status=@5xx' }
    draw:
      - { alias: '4xx', color: '#147', at: right }
      - { alias: '503', color: '#da7', at: left }
      - { alias: '5xx', color: '#b20', at: left }
```

Здесь `@4xx`, `@503` и `@5xx` – простая строковая подстановка некоторых общих значений.
В данном случае, это перечень ошибок из
[подключаемого файла](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/resources/dashboard/include/errors.yaml):
```yaml
variables:
  replacement:
    !include ../include/errors.yaml
```

#### [Percentile](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/PercentileTemplate.java)

![Percentile Graph](https://jing.yandex-team.ru/files/ssytnik/percentile.png "Percentile Graph")

Шаблон предназначен для построения перцентилей некоторой величины, обычно – длительностей запросов.

Исходные данные: гистограмма, представленная бакетами. Каждый бакет – отдельный сенсор. Допускается несколько сенсоров
на бакет, если к ним будет примена группировка (параметр `groupLines`). Тип значения сенсора определяется параметром `sensor`.
Допускаются `raw`-значения, `rate` и монотонно растущие счётчики – `counter`. В случае счётчиков, перед вычислением перцентилей,
сначала строится дельта гистограммы за интервал времени (с помощью производной по каждому бакету);

Параметры:
* `levels` (опциональный) – список требуемых перцентилей. По умолчанию равен `['50, '75', '90', '99']`;
* `format` (опциональный) – формат меток:
  * `prometheus` (по умолчанию) – к меткам добавляются: `hist_type=bin, le=*` (бакеты представлены меткой `le`);
  * `solomon` – к меткам добавлятся `bin=*` (бакеты представлены меткой `bin`);
  * `<V>` – к меткам добавляется `<V>=*` (бакеты представлены меткой `<V>`);  
* `groupLines` (опциональный) – `true` или `false` (по умолчанию). В случае `true`, линии явно группируются по метке бакета;
* `sensor` (опциональный) – указывает тип значения сенсора (см. [ниже](#тип-значения-сенсора)).

В шаблоне требуется указать ровно один `query`, который будет продублирован для каждого перцентиля из `levels`,
в том же порядке, при этом каждая линия (перцентиль) получит `alias: p<level>`, например: `p99`.
Также, в зависимости от формата (`format`), в `labels`будут дописаны необходимые метки, включая метку бакета.

По умолчанию, формат данных – секунды: `yAxes: [{ format: s }]`.

Пример:
```yaml
  - type: graph
    templates: { name: percentile, groupLines: true }
    title: 'Adapter response duration ($cluster)'
    queries:
      - params: { labels: 'name=grpc_durations' }
```

#### [PatchSelect](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/template/PatchSelectTemplate.java)

Позволяет изменить содержимое `select` в каждом `query`
(полезно для случаев, когда `select` формируется шаблоном неявно).
Сейчас поддерживается только добавление новых вызовов.

Указывается точка вставки и вставляемый набор вызовов функций.

Формат:
```yaml
  - type: graph
    templates: { name: patchSelect, <insertionPoint>, add: { fn1: [params1] ... } }
```

где `insertionPoint` – `before: <element>` или `after: <element>`,
а `<element>` – одно из: `{ name: first }`, `{ name: last }`, `{ name: <functionName> }`
или `{ index: 1 .. N }`.

Удобно задавать `<element>` в сокращённом виде (здесь `name` или `index` выбираются по типу):
`first | last | <functionName> | 1 .. N`. Например:
```yaml
  - type: graph
    templates: { name: patchSelect, before: last, add: { asap: [] } }
```

Если `select` отсутствует, считается, что он пуст.
В этом случае в качестве точки вставки можно указать `after: 0`, `after: last` или `before: first`.

### Общая информцаия по параметрам шаблонов

#### Тип значения сенсора

Каждый сенсор имеет тип, определяющий смысл его значений и способ работы с ним.
Тип задаётся в приложении, в агенте, в Solomon и при отображении графиков. В общем случае, эти типы не совпадают.
Дашборд-генератор работает со следующими типами значений:
- raw (текущее количество событий);
- rate (производная raw-значения);
- counter (монотонно растущий счётчик количества событий).

⚠ Параметр `sensor`, используемый в некоторых шаблонах, исторически по умолчанию имеет тип `counter`.

#### [Работа со значениями (downsampling, diff/derivative)](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/main/java/yandex/cloud/dashboard/model/spec/panel/SensorValueType.java)

Следующая таблица на примере типовых сценариев показывает, какие функции агрегации и первичного преобразования,
в зависимости от типа значения,
[выбирает генератор](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/src/test/java/yandex/cloud/dashboard/model/spec/panel/SensorValueTypeTest.java).

|          сценарий \ тип значения сенсора         | raw | rate                         |            counter             |
|:------------------------------------------------:|:---:|:----------------------------:|:------------------------------:|
| производная (шаблон rps)                         | –   | avg                          | max + nn_deriv*                |
| количество (шаблон errors)                       | sum | avg + integrate_fn + diff ** | max + nn_diff***               |
| относительное количество**** (шаблон percentile) | sum | avg (можно integrate)        | max + nn_deriv (можно nn_diff) |

`*`    – `nn_deriv` == `non_negative_derivative`  
`**`   – вместо этого выражения для downsampling в `groupByTime` лучше использовать `integrate`,
         но это пока [не поддерживается](https://st.yandex-team.ru/CLOUD-40480)   
`***`  – `nn_diff` == `diff` + `drop_below(0)`  
`****` – значение рассматривается относительно значений некоторого множества других сенсоров.
         Пример: бакеты гистограммы могут измениться в `Δt` раз, при этом результат `histogram_percentile` не изменится.
         Для простоты, правила для количества и относительного количества совпадают

Данные правила применяются в шаблонах. Однако, если шаблоны не используются, следует, исходя из типа значения сенсора,
выбирать группировку в downsampling (директива спецификации `groupByTime`) вручную.

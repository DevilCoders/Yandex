
# Миграция старых чартов

Изначально, ChartEditor умел выполнять код только в браузере.
Такой подход упрощал развитие сервиса, однако нес в себе множество недостатков:
- Сильно ограниченные объемы данных, доступные для загрузки и препроцессинга: когда на дашборде прогружается сразу несколько десятков чартов, даже 1 мегабайт в каждом из них становится проблемой
- Нюансы, связанные с разными браузерами их js-движками
- Небезопасность
- Мало возможностей по контролю с нашей стороны в целом: мы не могли отдавать результаты через API, имели весьма ограниченные возможности по сбору статистики и добавлению новых возможностей

В районе 2016 года мы начали делать первые подходы к препроцессингу на сервере (вычисляемые поля в визарде, отдельный таб для серверного кода в ChartEditor).
[Бета-тестирование](https://clubs.at.yandex-team.ru/statistics/1206) первой от и до работающей на сервере версии открылось 31 марта 2017 года.
Мы постепенно собирали обратную связь, добавляли к новому движку недостающие функции и поддержку всех видов чартов и проверяли все это в деле на ряде боевых дашборд.
[Запуск движка](https://clubs.at.yandex-team.ru/statistics/1403) произошёл 16 февраля 2018 года и мы сделали его средой выполнения по умолчанию для всех новых чартов.


## Основные моменты {#basic}

* ### Больше нет "магических" переменных, ChartEditor.setDataSource, ChartEditor.draw {#no-more-magic-variables}

```js
̶v̶̶a̶̶r̶̶ ̶̶p̶̶a̶̶r̶̶a̶̶m̶̶s̶̶ ̶̶=̶̶ ̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶;̶̶
```
```js
̶̶v̶̶a̶̶r̶̶ ̶̶g̶̶r̶̶a̶̶p̶̶h̶̶ ̶̶=̶̶ ̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶;̶̶
̶̶v̶̶a̶̶r̶̶ ̶̶t̶̶a̶̶b̶̶l̶̶e̶̶ ̶̶=̶̶ ̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶;̶̶
̶̶v̶̶a̶̶r̶̶ ̶̶m̶̶a̶̶p̶̶ ̶̶=̶̶ ̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶;̶̶
```
```js
̶̶v̶̶a̶̶r̶̶ ̶̶s̶̶t̶̶a̶̶t̶̶f̶̶a̶̶c̶̶e̶̶_̶̶g̶̶r̶̶a̶̶p̶̶h̶̶ ̶̶=̶̶ ̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶;̶̶
̶̶v̶̶a̶̶r̶̶ ̶̶s̶̶t̶̶a̶̶t̶̶f̶̶a̶̶c̶̶e̶̶_̶̶m̶̶e̶̶t̶̶r̶̶i̶̶c̶̶ ̶̶=̶̶ ̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶;̶̶
̶̶v̶̶a̶̶r̶̶ ̶̶s̶̶t̶̶a̶̶t̶̶f̶̶a̶̶c̶̶e̶̶_̶̶t̶̶e̶̶x̶̶t̶̶ ̶̶=̶̶ ̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶;̶̶
̶̶v̶̶a̶̶r̶̶ ̶̶s̶̶t̶̶a̶̶t̶̶f̶̶a̶̶c̶̶e̶̶_̶̶m̶̶a̶̶p̶̶ ̶̶=̶̶ ̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶;̶
```
```js
̶̶C̶̶h̶̶a̶̶r̶̶t̶̶E̶̶d̶̶i̶̶t̶̶o̶̶r̶̶.̶̶s̶̶e̶̶t̶̶D̶̶a̶̶t̶̶a̶̶S̶̶o̶̶u̶̶r̶̶c̶̶e̶̶(̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶)̶̶
̶̶C̶̶h̶̶a̶̶r̶̶t̶̶E̶̶d̶̶i̶̶t̶̶o̶̶r̶̶.̶̶d̶̶r̶̶a̶̶w̶̶(̶̶{̶̶ ̶̶.̶̶.̶̶.̶̶ ̶̶}̶̶)̶
```
...

Все вкладки реализуют **единый интерфейс экспорта**:

```js
module.exports = { ... };
```

Остальная логика на вкладке не меняется (за исключением других описанных здесь моментов).

**Было**

Вкладка Params:

```js
var params = {
    region: 'RU',
    scale: 'd'
}
```

Вкладка JavaScript:

```js
ChartEditor.draw({
    graphs: [{data: [1]}],
    categories_ms: [1577836800000]
});
```

**Стало**

Вкладка Params:

```js
module.exports = {
    region: 'RU',
    scale: 'd'
};
```

Вкладка JavaScript:

```js
module.exports = {
    graphs: [{data: [1]}],
    categories_ms: [1577836800000]
};
```

* ### Больше нет "глобальной" переменной params {#no-more-global-params}

Теперь необходимо явно получать параметры скрипта:

```js
const params = ChartEditor.getParams();
```

`params` будет следующего вида:

```js
{
  region: ['RU'],
  scale: ['d']
}
```

Чтобы получить значение одного параметра:

```js
const scale = ChartEditor.getParam('scale');
```

**Примечание**: все полученные значения параметров (включая одинарные) будут обернуты в массив, независимо от того, как их описывать на вкладке Params.

* ### Вместо вкладки Includes подключение модулей на вкладках {#instead-of-includes}

Подробнее про модули можно почитать в [соответствующем разделе](modules.md).

**Было**

Вкладка Includes:

```js
var includes = [
    'Statbox.Lambda',
    'aHelpers',
    'moment.js'
];
```

**Стало**

На каждой вкладке, где используется модуль:

```js
const _ = require('Statbox/Lambda');
const Helpers = require('modules/aHelpers');
const moment = require('vendor/moment/v2.24');
```

**Примечание**: рекомендуется отказаться от использования модулей семейства `Statbox`, т.к. поддержка по ним не осуществляется.

* ### Отдельные модули для источников вместо ChartEditor.buildUrl {#instead-of-buildurl}

Список доступных источников и примеры использования их модулей можно найти в [соответствующем разделе](sources/index.md#list-data-sources).

## Примеры {#examples}

[Examples/deprecated/parametrized-stat-graph](https://charts.yandex-team.ru/editor/Examples/deprecated/parametrized-stat-graph) -> [Examples/parametrized-stat-graph](https://charts.yandex-team.ru/editor/Examples/parametrized-stat-graph)
[Examples/deprecated/parametrized-stat-metric](https://charts.yandex-team.ru/editor/Examples/deprecated/parametrized-stat-metric) -> [Examples/parametrized-stat-metric](https://charts.yandex-team.ru/editor/Examples/parametrized-stat-metric)

## Legacy {#legacy}

Старая среда выполнения продолжит работать в уже существующих чартах (некоторые из которых написаны 7(!) лет назад),
однако исправляться в них будут только критические ошибки и мы не будем оказывать по ним поддержку в чате и на рассылках.

Также, чарты с браузерной средой выполнения нельзя будет использовать в наших новых разрабатываемых инструментах и сервисах.
Например, [новый интерфейс ChartEditor](https://clubs.at.yandex-team.ru/statistics/1796) и [Yandex.Dash](/docs/dash/migration).

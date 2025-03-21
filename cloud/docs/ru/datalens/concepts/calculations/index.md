# Вычисляемые поля

Вычисляемое поле — это дополнительное [поле данных](../dataset/index.md#field), значения которого вычисляются по формуле.
Вычисляемые поля позволяют вам создавать новые измерения и показатели.
При этом источник данных не изменяется.

Для записи формул вы можете использовать существующие поля датасета, [параметры](../parameters.md), константы и [функции](#functions).

Вы можете [создавать](#how-to-create-calculated-field) вычисляемые поля в интерфейсе датасета или визарда:
- Вычисляемые поля из датасета доступны во всех чартах над этим датасетом.
- Вычисляемые поля из визарда доступны только для сохраненного чарта.

## Вычисляемые поля в датасете {#dataset}

Вы можете добавлять новые поля в список полей датасета.

При создании формулы вы можете использовать любое поле датасета, в том числе вычисляемые поля, которые вы создали ранее. {% if audience == "internal" %}В формуле также можно использовать параметры, созданные на уровне датасета.{% endif %} 

После создания вычисляемого поля и сохранения датасета поле становится доступным для всех чартов и [селекторов](../dashboard.md#selector) дашбордов, которые построены на основе датасета.
Чтобы поля не отображались в визарде, при создании включите опцию **Не отображать**.

Вычисляемые поля помечаются значком ![image](../../../_assets/datalens/formula-dataset.svg).

## Вычисляемые поля в чарте {#chart}

Вы можете добавлять новые поля при создании и редактировании чарта в визарде:

- в списке полей из датасета;
- в секции визуализации.

При создании формулы вы можете использовать любое поле датасета, в том числе вычисляемые поля, которые вы создали ранее. {% if audience == "internal" %}В формуле также можно использовать параметры данного чарта или датасета, на основе которого создан чарт.{% endif %} 

Если поле создано в чарте, то оно не будет доступно для селекторов дашборда и других чартов.

Вычисляемые поля помечаются значком ![image](../../../_assets/datalens/formula-dataset.svg).

{% note warning %}

В [мультидатасетных чартах](../chart/index.md#multi-dataset-charts) вычисляемые поля над полями из нескольких датасетов неприменимы.

{% endnote %}

## Функции {#functions}

Функции — основной компонент создания формул. Они позволяют выполнять различные операции над полями данных.

Список доступных функций зависит от источника данных. Подробнее в разделе [{#T}](../../function-ref/availability.md).

{% note warning %}

Избегайте зацикливания вычислений: в формуле нельзя использовать поле, которое использует эту же формулу для вычисления своего значения.

{% endnote %}


### Формат записи {#entry-format}

Формулы должны быть записаны с учетом следующих требований:

1. Указывайте аргументы функций в круглых скобках. Например, `YEAR([DATE1])`. Несколько аргументов разделяются запятой.

1. Записывайте имена полей в квадратных скобках. Например, `[CustomerID]`.

1. Записывайте значения полей и констант в допустимом [формате](../dataset/index.md#data-types). Например, используйте точку в качестве разделителя для дробных чисел: `0.123`.

{% note info %}

- Синтаксис имен полей регистрозависимый. Например, если в формуле используется `[NAME]`, а в датасете есть только `[Name]`, поле в формуле будет подсвечено красным.
- Синтаксис функций регистронезависимый. Например, `count` и `COUNT` выполнят одну и ту же операцию.

{% endnote %}

Справочная информация по формату, типам принимаемых аргументов, а также возвращаемым значениям функций доступны в интерфейсе редактора формул.
Для этого нажмите кнопку **Справочник** в интерфейсе добавления поля.

## Как создать вычисляемое поле {#how-to-create-calculated-field}

Вы можете создать вычисляемое поле в интерфейсе датасета или визарда.

{% list tabs %}

- Датасет

  {% include [datalens-create-calculated-field-in-dataset](../../../_includes/datalens/operations/datalens-create-calculated-field-in-dataset.md) %}

- Визард

  1. Откройте [визард](https://datalens.yandex.ru/wizard/).
  1. Выберите датасет для создания чарта.
  1. В левой части экрана нажмите значок **![image](../../../_assets/plus-sign.svg)**, который находится над списком полей датасета.
  1. Введите необходимую формулу.
  1. Нажмите **Создать**. Поле отобразится в списке полей. Вы можете его использовать в качестве источника данных для чарта.

{% endlist %}

#### См. также {#see-also}
- [{#T}](../../operations/dataset/create-field.md)
- [{#T}](../../operations/dataset/manage-row-level-security.md)

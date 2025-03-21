# Политика распределения

{% if product == "cloud-il" %}

{% include [one-az-disclaimer](../../../../_includes/overview/one-az-disclaimer.md) %}

{% endif %}

При создании группы виртуальных машин можно выбрать в каких регионах, зонах доступности {{ yandex-cloud }} будут размещаться виртуальные машины.

Регион — это определенное географическое расположение, в котором можно развертывать виртуальные машины. Каждый регион имеет одну или несколько зон. На данный момент доступен только один регион. [Подробнее о географии {{ yandex-cloud }}](../../../../overview/concepts/geo-scope.md).

{% if product == "yandex-cloud" %}

Регион | Зоны | Географическая привязка
----- | ----- | -----
`{{ region-id }}` | `{{ region-id }}-a`<br/>`{{ region-id }}-b`<br/>`{{ region-id }}-c` | Владимирская, Московская и Рязанская области.

{% endif %}

{% if product == "cloud-il" %}

Регион | Зоны | Географическая привязка
----- | ----- | -----
`{{ region-id }}` | `{{ region-id }}-a` | Израиль, Тират-Кармель

{% endif %}

Переместить виртуальные машины в другие зоны доступности невозможно, но вы можете [обновить группу](../../../operations/instance-groups/deploy/rolling-update.md) с новыми значениями зон.

Политика распределения определяется в YAML-файле, в ключе `allocation-policy`. Значением может быть список зон, определенных в ключе `zones`. Вы можете указывать любые доступные зоны, между которыми будут равномерно распределяться виртуальные машины в группе.

Пример записи в YAML-файле:

```
...
allocation_policy:
    zones:
        - zone_id: {{ region-id }}-a
        - zone_id: {{ region-id }}-b
        - zone_id: {{ region-id }}-c
...
```

Где:

Ключ | Значение
----- | -----
`zones` | Список зон. Каждая зона задается в ключе `zone_id`, в виде пары `ключ:значение`.
`zone_id` | Идентификатор зоны.
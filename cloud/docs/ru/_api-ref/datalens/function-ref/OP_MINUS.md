---
editable: false
---

# Вычитание (-)



#### Синтаксис {#syntax}


```
value_1 - value_2
```

#### Описание {#description}
Имеет различное поведение в зависимости от типов аргументов. Возможные варианты приведены в таблице:

| Тип `value_1`                                 | Тип `value_2`                                 | Возвращаемое значение                                                                                                                                                               |
|:----------------------------------------------|:----------------------------------------------|:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| <code>Дробное число &#124; Целое число</code> | <code>Дробное число &#124; Целое число</code> | Разность чисел `value_1` и `value_2`.                                                                                                                                               |
| `Дата`                                        | <code>Дробное число &#124; Целое число</code> | Дата, на `value_2` дней меньшая, чем `value_1` (с округлением вниз до целого количества дней).                                                                                      |
| `Дата и время`                                | <code>Дробное число &#124; Целое число</code> | Дата со временем, на `value_2` дней меньшая, чем `value_1`. Если `value_2` содержит дробную часть, то она пересчитыватся в часы (`1/24`),  минуты (`1/1440`) и секунды (`1/86400`). |
| `Дата`                                        | `Дата`                                        | Разность двух дат в днях.                                                                                                                                                           |
| `Дата и время`                                | `Дата и время`                                | Разность двух дат в днях: целая часть — количество целых дней, дробная — количество часов, минут и секунд как долей целого дня (час — `1/24` и т.д.).                               |

**Типы аргументов:**
- `value_1` — `Дата | Дата и время | Дробное число | Целое число`
- `value_2` — `Дата | Дата и время | Дробное число | Целое число`


**Возвращаемый тип**: Зависит от типов аргументов

#### Примеры {#examples}

```
2 - 3 = -1
```

```
2 - 0.5 = 1.5
```

```
#2019-01-06# - 2 = #2019-01-04#
```

```
#2019-01-06# - 2.2 = #2019-01-03#
```

```
#2019-01-06 03:00:00# - 2 = #2019-01-04 03:00:00#
```

```
#2019-01-06 03:00:00# - 2.5 = #2019-01-03 15:00:00#
```

```
#2019-01-06# - #2019-01-02# = 4
```

```
#2019-01-06 15:00:00# - #2019-01-02 03:00:00# = 4.5
```


#### Поддержка источников данных {#data-source-support}

`Материализованный датасет`, `ClickHouse 19.13`, `Yandex.Metrica`, `Microsoft SQL Server 2017 (14.0)`, `MySQL 5.6`, `Oracle Database 12c (12.1)`, `PostgreSQL 9.3`, `YDB`.

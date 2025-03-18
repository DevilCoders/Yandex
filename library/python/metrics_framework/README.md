# metrics_framework
Фреймворк для сбора метрик с возможностью загрузки в stat

Как использовать:
1) Обернуть функцию подсчёта метрики декоратором `@metric('metric_slug')`
2) Создать в базе объект `Metrics` с таким же слагом
#



В зависимости от структуры отчета, функция подсчета метрики должна возвращать данные в одном из двух форматов:
1) Для отчетов где одно дефолтное измерение `fielddate` возвращается список словарей, содержащих пары с ключами `slug` - название столбца в stat и `value` - числовое значение:
    ```
    @metric('foo')
    def foo():
        return [
            {'slug': 'measure1_name', 'value': number},
            {'slug': 'measure2_name', 'value': number},
            ...
        ]
    ```
2) Для отчётов, имеющих несколько измерений - возвращать список словарей, содержащих пары с ключами`context` - словарь, задающий значения дополнительных измерений в формате `dimension_name: dimension_value`
и `values` - список словарей, содержащих пары с ключами `slug` - название столбца в stat и `value` - числовое значение
    ```
    @metric('bar')
    def bar():
        return [
            {
                'context': {
                    'dimension1_name': 'dimension1_value1',
                    'dimension2_name': 'dimension2_value1',
                    ...
                },
                'values': [
                    {'slug': 'measure1_name', 'value': number},
                    {'slug': 'measure2_name', 'value': number},
                    ...
                ]
            }
            {
                'context': {
                    'dimension1_name': 'dimension1_value2',
                    'dimension2_name': 'dimension2_value2',
                    ...
                },
                'values': [
                    {'slug': 'measure1_name', 'value': number},
                    {'slug': 'measure2_name', 'value': number},
                    ...
                ]
            }
        ]
    ```

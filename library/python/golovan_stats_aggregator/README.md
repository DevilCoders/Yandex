Обертка для формирования unistat ручки и сбора параметров для нее.



Установка:

    pip install -i https://pypi.yandex-team.ru/simple golovan_stats_aggregator


Как использовать:

```python
from golovan_stats_aggregator import MemoryStatsAggregator

stats_aggregator = MemoryStatsAggregator()

...

  try:
    send_message(msg)
  except Exception:
    stats_aggregator.inc('errors_count_summ')

```

После чего нужно отдавать по адресу unistat-ручки данные из `stats_aggregator.get_data()`

Для приложений с несколькими процессами под `uwsgi` есть специальный аггрегатор `UwsgiStatsAggregator`:

```
from golovan_stats_aggregator.uwsgi import UwsgiStatsAggregator
```

Он требует настройки cache2 в вашем конфиге uwsgi:

```
cache2 = name=my_cache,items=10000
```

[Алерт oct-memory-available в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Doct-memory-available)

## Что проверяет

- Наличие достаточного запаса MemAvailable на OCT инстансе
  MemAvailable: An estimate of how much memory is available for starting new applications, without swapping.
  Calculated from MemFree, SReclaimable, the size of the file LRU lists, and the low watermarks in each zone.

## Если загорелось

- проверяем, не горит ли *-memory мониторинг какого-то сервиса (сначала стоит разобраться с ним)

- проверяем потребление памяти сервисами запущеными на проблемном инстансе

- проверяем нагрузку (нет ли большого числа запросов, изменений, сообщений в логах сервиса)

- проверяем по графикам памяти, нет ли плавного роста потребления, тогда, возможно, это утечка

- если часто загорается, стоит завести тикет
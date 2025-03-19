[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dgpu-servicevm)

## gpu-servicevm
Проверяет, что сервис `yc-gpu-servicevm` работает и может принимать запросы
(то есть здоров, `healthy`).

## Подробности
Сервис `yc-gpu-servicevm` используется на машинах с NVIDIA NVswitch.

Если сервис `yc-gpu-servicevm` не работает, то на этом узле невозможны запуск
новых виртуальных машин с GPU, остановка или перезапуск (рестарт)
существующих. Это так, потому что при запуске или остановке пользовательской
виртуальной машины должны выполняться соответственно активация и деактивация
NVswitch разделов (NVswitch partition).

Считаем, что отказ `yc-gpu-servicevm` не влияет на `NVLINK dataplane`, но
только в том случае, если запущен и нормально функционирует сервис
`yc-gpu-fdkeeper`: команда `sudo yc-gpu-fdkeeperctl list` должна работать и
выдавать `set` с именем `yc-gpu-nvswitch`. Если сервис `yc-gpu-fdkeeper` также
сломан, то нужно экстренное расселение узла через `stop/start`.

## Диагностика
Если сервис остановлен (по статусу `systemd`), то безопасно попробовать его
запустить. Если сервис не запустился, то читай раздел "Если сломалось".

В Juggler-событиях в описании (`description`) прилетает строка вида:

    health: healthy, state: loaded, fabric manager: activated, started, running

Что мы считаем `healthy`:

    health: healthy, state: loaded, fabric manager: activated, started, running
    health: healthy, state: loaded, fabric manager: activated/not activated, not started, not running

Все остальные комбинации - `unhealthy`, смотри раздел "Если сломалось".

Поле `health` отражает общее здоровье сервиса:

  - `healthy` - здоров
  - `unhealthy` - болен (смотри остальные компоненты статусной строки)
  - `shutting-down` - останавливается (не считаем проблемой, но если
    останавливается тольше разрешенного, то загориться сначала `Warn`, потом
    `Crit`)
  - `failure` - переведен в `failure mode` командой `yc-gpu-fmproxyctl failure-mode`.
    Смотри раздел "Failure mode".

Поле `state` показывает - загружен ли `FabricMananger-state` файл в сервисную
виртуальную машину. Если `state: not loaded`, значит сервисная виртуальная
машина не смогла запуститься. Нужно смотреть логи сервиса `yc-gpu-servicevm`.

Поле `fabricmanager` показывает:
  - активированы ли разделы (`activated` или `not activated`). Compute Node
    должна послать список активных разделов как только сможет подключиться по
    gRPC к сервисной виртуальной машине. Если здесь `not activated`
    - проверить, запущен ли сервис `yc-compute-node`
    - посмотреть логи сервиса `yc-compute-node`, чтобы понять, по какой
      причине не отправлен список активных разделов
  - был запущен ли процесс `NVIDIA FabricManager` (`started` или `not
    started`). Здесь имеется ввиду - был ли запущен процесс `NVIDIA
    FabricManager` в принципе. Актуальный статус процесса `NVIDIA
    FabricManager` показан в компоненте `running`/`not running`.
    - если `NVIDIA FabricManager` - `not started, not running` - это
      **норма**. Процесс `NVIDIA FabricManager` будет запущен при первой
      активации или деактивации любого раздела.
    - если `NVIDIA FabricManager` - `started, not running` - произошла
      критическая ошибка. Смотри раздел "Если сломалось".

Поле `locked partitions` показывает, для каких разделов больше разрешенного
бегут операции Activate, Deactivate или Unplug partition. Например:

    state: loaded, fabric manager: activated, started, running, locked partitions: #1 since 14:03:22 (activating)

В этом сообщение написано: раздел `1` активируется неприлично долго. В этом
случае нужно смотреть логи сервиса `yc-gpu-servicevm`.

## Если сломалось

Если сервис `yc-gpu-servicevm` не запускается или `unhealthy`, то

1) Выключить узел в планировщике

2) Если узел пустой, то выполнить процедуру восстановления после сбоя

3) Если узел не пустой

   3.1) Если `yc-gpu-fdkeeper` исправен, то оповещаем пользователей, что им
   надо уехать через `stop/start`

   3.2) Если `yc-gpu-fdkeeper` неисправен, то эвакуируем виртуальные машины с
   узла и переходим к процедуре восстановления после сбоя.

## Процедура восстановления после сбоя

Выполняется только, если
  - узел выключен в планировщике
  - узел без виртуальных машин с GPU
  - известна причина сбоя и эта причина позволяет проводит процедуру восстановления после сбоя

`yc-compute-node` останавливать не обязательно.

1. Остановить `yc-gpu-fdkeeper` и `yc-gpu-servicevm`

        sudo systemctl stop yc-gpu-fdkeeper yc-gpu-servicevm

2. Удалить `fabricmanager.state`

        rm /run/yc/gpu/service-vm/fabricmanager.state

3. Запустить `yc-gpu-servicevm`

        sudo systemctl start yc-gpu-servicevm

4. Дождаться, когда `yc-gpu-servicevm` станет `healthy`.

Можно возвращать узел в строй.

## Failure Mode

Сервис `yc-gpu-servicevm` может быть переведен в `failure mode`, в котором
разрешена только деактивация разделов (хотя фактически деактивация выполняться
не будет). Этот режим нужен для расселения сломанного узла. Включается `failure mode` командой

    yc-gpu-fmproxyctl failure-mode

Перед ключением `failure mode` узел должен быть выключен в планировщике, в
противном случае все операции старта будут падать с ошибкой.

Учитывая то, что деактивация разделов в `failure mode` выполняться не будет,
после включения этого режима узел должен быть предварительно починен, перед
возвращением его в строй. Как минимум, нужно сбросить все NVswitch'и и GPU
карты (то есть, на узле не должно остаться гостей).

В экстренном случае (не делай так никогда), можно вывести сервис из `failure
mode` командой

    yc-gpu-fmproxyctl failure-mode --disable

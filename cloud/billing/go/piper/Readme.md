# piper

Это сервис, предназначенный для обработки метрик биллинга Яндек.Облака.

## Структура проекта

### cmd/piper

Тут распололжен код основного исполняемого файла. Содержится минимум логики про запуск и считывание параметров.

### configuration

Каталог, предназначенный для поставки конфигураций сред при деплое.
Основные конфиги для всех сред кладутся в конкретные [каталоги](piper/configuration/configs) и пакуются в докер-образ.
При запуске этот образ копирует конфиги из выбранного каталога в общий volume и монтирует его ко всем остальным контейнерам, см. [pod-файл](deploy/packer/piper/files/piper.pod.tmpl).

### package

Тут расположено описание докер-образа самого сервиса.

Собранный сервис ожидает подмонтированные конфиги и не содержит дополнительной логики по конфигурации. См. [pod-файл](deploy/packer/piper/files/piper.pod.tmpl).

### internal
Здесь расположена реализация логики сервиса.

## Деплой

Для развертывания в Яндекс.Облаке в качестве конечного артефакта собирается compute образ, единый для всех окружений. Далее собранный образ копируется во все инсталляции
облака (PREPROD, PROD, GPN) и используется для запуска виртуальных машин с сервисом. Выбор настроек и дополнительная конфигурация производится с помощью метаданных виртуальной машины и сервисов инсталляции облака (lockbox).

В образе compute запускается kubelet, который настроен без использования мастера и ожидает описание сервисов в виде файла с [pod-spec описанием](https://kubernetes.io/docs/reference/generated/kubernetes-api/v1.19/#pod-v1-core), см. [pod-файл](deploy/packer/piper/files/piper.pod.tmpl).
Базовый образ и детали его построения и конфигурации описаны в репозитории, используем версию [paas-base-g4](https://bb.yandex-team.ru/projects/CLOUD/repos/paas-images/browse/paas-base-g4).

Докер-образы скачиваются в compute образ при сборке и больше автоматически не обновляются. Связано это с фиксацией версий кода и настроек, а так же с недоступностью оригинального репозитория в инсталляцияз, в которые копируется compute обаз.

Развертывание виртуальных машин производится с помощью описания [terraform](deploy/terraform). Обновление виртуальных машин должно производиться после тиражирования образа путем указания его в настройках сред и применения описания.

### Сборка докер образов

Сборка происходит в [сборочном цехе](https://wiki.yandex-team.ru/cloud/devel/assembly-workshop/welcome) с помлощью настроенных билдов:

#### Сборка основного образа

Расположена тут: https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=Billing_ArcBuildPiper

Для сборки новой версии поднимаем номер версии в файле [version.json](piper/package/version.json), коммитим и запускаем сбоку в teamcity. В перспективе это можно будет автоматизировать.

#### Сборка образа конфигураций

Расположена тут: https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=Billing_ArcBuildPiperConfigs

Собирать надо в 2 случаях:
- выход новой версии кода - дополнительно изменяем версию в [version.json](piper/configuration/version.json)
- обновление только конфигурации - менять версию не надо, т.к. в версию образа подставляется ревизия, из которой собрался образ.

Сборку запускаем в teamcity.

#### Сборка дополнительных инструментов

Сервис требует для работы некоторые дополнительные инструменты (jaeger-agent и unified-agent на текущий момент).
Они поставляются как отдельные образ с собранными из аркадии бинарниками.

Релиз этого образа не привязан к релизам кода или конфигов и делается по мере накопления существенных изменений в утилитах.

Перед сборкой надо обновить версию в [version.json](deploy/tools/version.json) Сборка производится с помощью билда в teamcity https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=Billing_ArcBuildToolsDocker

### Сборка и копирование compute образа

Расположена тут: https://teamcity.aw.cloud.yandex.net/viewType.html?buildTypeId=Billing_ArcPiperPacker

Копирование пока не реализовано, см. https://wiki.yandex-team.ru/cloud/devel/assembly-workshop/welcome/#packer-with-hopper для деталей реализации

Для сборки образа используется [packer-рецепт](deploy/packer/piper/biling-piper.pkr.hcl). Рядом расположены файлы с настройками:
- [aw.pkrvars.hcl](deploy/packer/piper/aw.pkrvars.hcl) - настройки среды запуска в сборке (определяются настройками сборочного цеха)
- [image_versions.pkrvars.hcl](deploy/packer/piper/image_versions.pkrvars.hcl) - версии докер-образов для билда

Для сборки нового релиза надо актуализировать версии в image_versions.pkrvars.hcl. Собранные версии образов можно проверить в [реестре](https://console.cloud.yandex.ru/folders/yc.billing.service-folder/container-registry/crpb8mold3ptghdke34l). После коммита можно запускать таски в teamcity.

## Конфигурация сервиса

Для задания конфигруации используется несколько систем:
- локальные файлы, путь к которым передан в параметрах `--config` запуска сервиса (принимаются glob'ы)
- переменные окружения (соответствующие ключу конфигурации)
- lockbox секрет, идентификатор которого установлен в ключе `lockbox-secret-id` метаданных виртуальной машины
- таблица `utility/context` в базе ydb

Порядок загрузки:
- загружается конфигурация из файлов и переменных окружения
- определяется разрешена ли загрузка конфигурации из lockbox в переданных файлах конфигурации и переменных окружения (ключ `lockbox.enabled`)
- если загрузка из lockbox разрешена, то получается значение `lockbox-secret-id` из метаданных и если он установлен, то загружается конфигурация из lockbox (и переменных окружения)
- производится подключение к ydb и загружается таблица с конфигурацией. Значениями из базы могут быть переопределны только [некоторые ключи](piper/internal/config/configuration_override.go)

### Значения по умолчанию

В коде для конфигураций есть некоторые значения по умолчанию. Их можно посмотреть или в пактете `internal/config` или запустив исполняемый фай без параметров конфигурации с подкомандой `show-config`

### Ключи конфигурации

Конфигурация сервиса представляет собой иерархическую структуру с вложенными объектами. Для переопределения конкретного значения в конфигурации используется перечисление
частей пути к значению через точку.

Например, такой ключ `logbroker_installations.cloud.port` будет соответствовать такому значению в структуре:
```(yaml)
logbroker_installations:
  cloud:
    port: 1111
```

### Конфигурация с помощью файлов

Для конфигурации сервиса используются yaml файлы. Можно перечислять несколько файлов, при этом загрузка производится последовательно в порядке перечисления с перезаписью значений.

### Конфигурация из lockbox

Секрет lockbox представляется сообой key-value зашифрованное хранилище. Для загрузки конфигурации используется:
- файловый ключ `yaml`, который загружается сразу после локальных файлов с перезаписью значений
- ключи, начинающиеся с префикса `piper.`, которые переопределяют индивидуальные значения.

### Конфигурация из ydb

После подключения к базе загружается содержимое таблицы и ключи, начинающиеся с `piper.` используются для переопределения ключей конфигурации.

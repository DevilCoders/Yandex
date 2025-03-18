Nanny
=====

Тасклеты для релиза sandbox-ресурсов и docker-образов в Nanny.

***Внимание:***
- Если вы ранее пользовались релизной (тикетная) интеграцией в сервисе, включая таковую для тасклета Релизной Машины `common/nanny/release`, то эти релизные правила надо удалить: при выкатке будет случаться race condition между выкладкой, вызванной CI, и выкладкой из-за релизного правила. Как результат, тасклет будет падать в случайные запуски (`Changes already present in service` или `Deploy failed with status: CANCELLED`).
- Если у вас в сервисе настроена Scheduling policy, Nanny может начать применять соответствующую логику к создаваемым снапшотам (например, начать активировать снапшот, который ещё не должен активироваться), это будет приводить к падению тасклетов.  Для релизов через CI крайне рекомендуется отключить Scheduling policy в настройках сервиса в Nanny.

***NOTE:*** этот файл описывает новые тасклеты от команды Деплоя. Документация на старые тасклеты лежит в файле `README.deprecated.md`.

***NOTE:*** обо всех найденных проблемах сообщайте в [тикеты в RTCSUPPORT][rtcsupport].

- [Получите токен][get-token]
- Положите с ключом `nanny_ci.token` в [секрет CI][ci-secret] в [Секретнице][yav]
- Проверьте, что у робота есть права на обновление сервиса в Nanny, если нет, их необходимо выдать
- Добавьте во flow запуск `common/nanny/update_service`
    - Сконфигурируйте кубик, указав `service_id` и список патчей (или докер-образ), описывающих, какими ресурсами патчить какие части стейджа.  
      При необходимости укажите идентификаторы рецептов `prepare_recipe` и `activate_recipe`, используемых для выкладки сервиса.  
      Структура патчей соответствует proto-сообщению [Patch][nanny-patch-spec]. [Примеры патчей](#patches-examples) смотрите ниже.
    - Опционально можно указать очередь Няни `queue_id`, в которую будут помещаться создаваемые тикеты.

## Полокационные аппрувы {#location-approves}

Для полокационных аппрувов в качестве пререквизита необходимо использовать специальный рецепт активации, который умеет специальным образом оперировать локациями. Технически можно любой рецепт адаптировать для использования в CI[¹](#recipe-diy), но на данный момент есть только один специально подготовленный рецепт [_activate_service_configuration_locationwise_auto.yaml](recipe), который позволяет производить полокационную активацию снапшота и использует автоматическое определение списка используемых кластеров. Его необязательно добавлять в сервис, можно просто включить опцией в кубике:
```yaml
config:
  default_activate_recipe:
    recipe: LOCATIONWISE
    manual_confirmation: true
```

***NOTE:*** Gencfg нашим рецептом не поддерживается. Только YPLite.

После этого нужно необходимое количество кубиков `common/nanny/approve_location`, которые будут использоваться во flow для одобрения выкладки в локацию.

Примеры смотрите ниже ([раз](#approve1), [два](#approve2)).

## Примеры конфигураций {#patches-examples}

Полные примеры можно посмотреть [в аркадии][examples]. Ниже приведены выдержки из этих примеров, демонстрирующие непосредственно флоу выкладки.

### Сборка и выкладка статического ресурса

```yaml
flows:
  autobuild:
    jobs:
      # Кубик (или кубики), описывающий сборку.
      # На выходе получается произвольное множество объектов SandboxResource,
      # которые могут использоваться на стадии выкладки.
      build:
        task: common/arcadia/ya_make
        title: Build
        stage: build
        input:
          targets: infra/nanny_ci/examples/static_resource_no_approves
          arts: infra/nanny_ci/examples/static_resource_no_approves/http_server
          result_rt: TEST_RESOURCE
          result_single_file: true
          build_type: release
      # Кубик для выкладки собранных артефактов в няню.
      release-to-nanny:
        task: common/nanny/update_service
        title: Update service
        needs: build
        stage: deploy
        input:
          config:
            # Имя сервиса, который мы обновляем 
            service_id: test_ci_torkve
            # Список применяемых патчей. В данном случае мы собираем и обновляем только один ресурс.
            patches:
              - sandbox:
                  # Ищем собранный артефакт типа TEST_RESOURCE
                  resource_type: TEST_RESOURCE
                  # и обновляем им ресурс с именем example_http_server в няне
                  resource:
                    local_path: example_http_server
```

### Сборка и обновления слоя

```yaml
flows:
  autobuild:
    jobs:
      # Кубик (или кубики), описывающий сборку.
      # На выходе получается произвольное множество объектов SandboxResource,
      # которые могут использоваться на стадии выкладки.
      build:
        task: common/arcadia/ya_package_2
        title: Build layer
        stage: build
        input:
          # Путь относительно корня аркадии к спеке собираемого с помощью ya package пакета
          packages: infra/nanny_ci/examples/layer_no_approves/build.json
          package_type: tarball
          resource_type: TEST_RESOURCE
      # Кубик для выкладки собранных артефактов в няню. 
      release-to-nanny:
        task: common/nanny/update_service
        title: Update service
        needs: build
        stage: deploy
        input:
          config:
            # Имя сервиса, который мы обновляем 
            service_id: test_ci2_torkve
            # Список применяемых патчей. В данном случае мы собираем и обновляем только один ресурс.
            patches:
              - sandbox:
                  # Ищем собранный артефакт типа TEST_RESOURCE
                  resource_type: TEST_RESOURCE
                  # и обновляем им слой с идентификатор http_layer в няне.
                  # NOTE: по умолчанию слои в Няне не имеют никаких идентификаторов. Чтобы их
                  # можно было обновлять при помощи CI, нужно зайти в instance spec сервиса в
                  # няне, нажать кнопку редактирования соответствующего слоя и присвоить ему
                  # произвольный идентификатор.
                  layer:
                    id: http_layer
```

### Сборка и выкладка docker-образа с помощью ya package

Для сборки docker-образа при помощи задачи YA_PACKAGE необходимо предварительно подготовить Dockerfile и json-спецификацию сборки [по инструкции][docker-ya-package].
Пример Dockerfile и файла build.json можно посмотреть [в примерах][docker-example].

Кроме того в необходимо выдать роботу **robot-qloud-viewer@** роль **viewer** на ваш docker-репозиторий в registry.

Для docker необходимо указывать при сборке и выкладке имя тега (как правило для него используется версия), поэтому здесь приводится пример со сборкой Docker-образа в релизном flow, где доступно поле `${context.version_info}`:

```yaml
flows:
  autobuild:
    jobs:
      # Кубик, описывающий сборку.
      build:
        task: common/arcadia/ya_package_2
        title: Build docker
        stage: build
        input:
          # Путь относительно корня аркадии к спеке собираемого с помощью ya package пакета
          packages: infra/nanny_ci/examples/docker_no_approves/build.json
          package_type: docker
          # Имя docker-репозитория. Это не имя docker-образа: если вы заливаете свой
          # пакет в докер, например, как nanny-ci/http_server_example:1.0
          # то "nanny-ci" — это репозиторий, "http_server_example" — имя образа,
          # а "1.0" — имя тега.
          docker_image_repository: nanny-ci
          # Имя робота, который авторизуется в Registry
          docker_user: robot-drug-deploy
          # Имя хранилища Sandbox Vault, содержащего OAuth-токен для доступа docker_user к registry.
          # В качестве токена в Vault можно сохранить тот же токен, который вы сохранили
          # под именем nanny_ci.token в yav. 
          docker_token_vault_name: docker.registry.token
          docker_push_image: true
          # Явно задаём имя тега, который мы собираем. Оно потребуется ниже.
          custom_version: "${context.version_info.full}"
      # Кубик для выкладки собранного образа в няню. 
      release-to-nanny:
        task: common/nanny/update_service
        title: Update service
        needs: build
        stage: deploy
        input:
          config:
            # Имя секрвиса, который мы обновляем.
            service_id: test_ci3_torkve
            # Полное имя применяемого docker-образа, включая
            # имя тега (совпадает с тем тегом, который мы собрали выше).
            docker_image: "nanny-ci/example_http_server:${not_null(context.rollback_to_version.full, context.version_info.full)}"
```

### Раздельная подготовка и активация конфигурации

Если необходимо сначала сделать prepare новой конфигурации, выполнить какие-то дополнительные операции, и только потом её активировать, то это возможно, для этого кубику `common/nanny/update_service` можно указать, что сервис не нужно активировать, а для активации использовать отдельный кубик `common/nanny/set_snapshot_state`, который меняет меняет состояние снапшота.

```yaml
flows:
  autobuild:
    jobs:
      # Кубик (или кубики), описывающий сборку.
      # На выходе получается произвольное множество объектов SandboxResource,
      # которые могут использоваться на стадии выкладки.
      build:
        task: common/arcadia/ya_package_2
        title: Build layer
        stage: build
        input:
          packages: infra/nanny_ci/examples/layer_no_approves/build.json
          package_type: tarball
          resource_type: TEST_RESOURCE
      # Кубик для подготовки нового снапшота в няне.
      prepare-service:
        task: common/nanny/update_service
        title: Prepare service
        needs: build
        stage: deploy
        input:
          config:
            # Имя секрвиса, который мы обновляем.
            service_id: test_ci4_torkve
            # Состояние в котором должен оказаться новый снапшот.
            # По умолчанию это ACTIVE, но здесь мы явно говорим,
            # что надо остановиться на PREPARED.
            target_status: PREPARED
            patches:
              - sandbox:
                  resource_type: TEST_RESOURCE
                  layer:
                    id: http_layer
      # Отдельное действие для активации снапшота.
      activate-service:
        task: common/nanny/set_snapshot_state
        title: Activate service
        needs: prepare-service
        stage: deploy
        input:
          config:
            # Состояние, в которое необходимо перевести сервис
            target_status: ACTIVE
            # Рецепт, с помощью которого необходимо активировать конфигурацию
            activate_recipe: common
```

Версию flow с несколькими сервисами смотрите в [примере][example-set-snapshot]

### Выкладка ресурса с произвольным порядком полокационных аппрувов {#approve1}

```yaml
flows:
  autobuild:
    jobs:
      # Кубик (или кубики), описывающий сборку.
      # На выходе получается произвольное множество объектов SandboxResource,
      # которые могут использоваться на стадии выкладки.
      build:
        task: common/arcadia/ya_make
        title: Build
        stage: build
        input:
          targets: infra/nanny_ci/examples/static_resource_with_multiplied_approves
          arts: infra/nanny_ci/examples/static_resource_with_multiplied_approves/http_server
          result_rt: TEST_RESOURCE
          result_single_file: true
          build_type: release
      # Кубик для выкладки собранного образа в няню.
      release-to-nanny:
        task: common/nanny/update_service
        title: Update service
        needs: build
        stage: deploy
        input:
          config:
            # Имя секрвиса, который мы обновляем.
            service_id: test_ci5_torkve
            # Настройки рецепта, который мы используем для выкатки.
            # Смотрите раздел "Полокационные аппрувы" в документации выше.
            default_activate_recipe:
              recipe: LOCATIONWISE
              manual_confirmation: true
            patches:
              - sandbox:
                  resource_type: TEST_RESOURCE
                  resource:
                    local_path: example_http_server
      # Кубик для одобрения выкатки локаций.
      # Поскольку у нас нет строгого порядка одобрения локаций, 
      # мы используем конструкцию multiply-by, создающую равнозначные кубики.
      approvals:
        task: common/nanny/approve_location
        needs: release-to-nanny
        stage: deploy
        multiply:
          by: ${tasks.release-to-nanny.approval_locations}
          title: "Approving location ${by.cluster}"
        input:
          location_match: ${by}
        manual:
          enabled: true
          prompt: "Approve location ${by.cluster}?"
```

### Выкладка ресурса со строгим порядком полокационных аппрувов {#approve2}

```yaml
flows:
  autobuild:
    jobs:
      # Кубик (или кубики), описывающий сборку.
      # На выходе получается произвольное множество объектов SandboxResource,
      # которые могут использоваться на стадии выкладки.
      build:
        task: common/arcadia/ya_make
        title: Build
        stage: build
        input:
          targets: infra/nanny_ci/examples/static_resource_with_sequential_approves
          arts: infra/nanny_ci/examples/static_resource_with_sequential_approves/http_server
          result_rt: TEST_RESOURCE
          result_single_file: true
          build_type: release
      # Кубик для выкладки собранного образа в няню.
      release-to-nanny:
        task: common/nanny/update_service
        title: Update service
        needs: build
        stage: deploy
        input:
          config:
            # Имя секрвиса, который мы обновляем.
            service_id: test_ci6_torkve
            # Настройки рецепта, который мы используем для выкатки.
            # Смотрите раздел "Полокационные аппрувы" в документации выше.
            default_activate_recipe:
              recipe: LOCATIONWISE
              manual_confirmation: true
            patches:
              - sandbox:
                  resource_type: TEST_RESOURCE
                  resource:
                    local_path: example_http_server
      # Кубик одобрения выкатки в первую локацию (man)
      approve-man:
        task: common/nanny/approve_location
        title: Approve location man
        needs: release-to-nanny
        stage: deploy
        input:
          location_match:
            # Указываем кластер, выкладку в который мы одобряем
            cluster: man
            # Также нужно указать имя сервиса, если в рамках одного flow
            # мы выкатываем несколько сервисов:
            # service_id: test_ci6_torkve
        manual:
          enabled: true
          prompt: "Approve location man?"
      # Кубик одобрения выкатки во вторую локацию (sas)
      approve-vla:
        task: common/nanny/approve_location
        title: Approve location vla
        needs: approve-man
        stage: deploy
        input:
          location_match:
            # Указываем кластер, выкладку в который мы одобряем
            cluster: vla
            # Также нужно указать имя сервиса, если в рамках одного flow
            # мы выкатываем несколько сервисов:
            # service_id: test_ci6_torkve
        manual:
          enabled: true
          prompt: "Approve location vla?"
```

## Подробные параметры кубиков

Самые актуальные параметры с описаниями всегда лежат в соответствуюших protobuf-спеках в аркадии (message `Input`):
* [common/nanny/update_service][update-service-proto]
* [common/nanny/set_snapshot_state][set-snapshot-state-proto]
* [common/nanny/approve_location][approve-location-proto]

### common/nanny/update_service

Основной кубик для создания релиза и применения его к сервису.

* `sandbox_resources`  
   По умолчанию это поле автоматически конструируется из всех ресурсов, которые были собраны задачами на предыдущих этапах flow (такими как ya_make и ya_package).  
   Однако в экзотических сценариях вы можете самостоятельно заполнить это поле вручную или при помощи JMESPath.
* `config`  
   Все настройки кубика.
    * `nanny_installation`  
      Необязательное поле с адресом инсталляции Няни. Требуется указывать только в ситуациях, если вы выкладываетесь через нестандартную инсталляцию (adm-nanny или dev-nanny).
    * `service_id`  
      Обязательный идентификатор сервиса в Няне.
    * `queue_id`  
      Необязательно поле с указанием очереди в Няне, в которую помещать создаваемые тикеты. Как правило, его менять не нужно. По умолчанию используется очередь "CI".
    * `release_title`  
      Необязательное поле, задающее заголовок релиза, создаваемого в Няне.
    * `release_description`  
      Необязательное поле, задающее описание релиза, создаваемого в Няне.
    * `release_type`  
      Необязательное поле, задающее тип релиза, создаваемого в Няне.  
      Как правило не несёт никакого смысла (так, как релизная интеграция не используется), но возможно может как-то помочь группировать релизы по типам при разных флоу.
    * `target_status`  
      Необязательное поле, указывающее, в какой статус перевести созданный релизом снапшот.  
      Возможные варианты:  
          1. **COMMITTED** (сохранить изменения только в Няне, но ничего с ними не делать)  
          2. **PREPARED** (выкатить конфигурацию, но не переключать сервис на неё)  
          3. **ACTIVE** (выкатить конфигурацию и переключиться).  
      По умолчанию используется **ACTIVE**.
    * `prepare_recipe`  
      Идентификатор рецепта в сервисе в Няне, которым осуществляется prepare конфигурации.  
      При отсутствии значения или указании невалидного идентификатора будет использоваться стандартный встроенный рецепт, который подходит большинству сервисов.
    * `default_activate_recipe`  
      Использовать стандартный рецепт для активации сервиса.  
      Нельзя использовать одновременно с настройкой `activate_recipe`.  
      При использовании стандартных рецептов настройка `activate_prefix` игнорируется.  
      По умолчанию используется стандартный рецепт **LOCATIONWISE**.
        * `recipe`  
          Тип стандартного рецепта. Поддерживается только значение **LOCATIONWISE**.
        * `manual_confirmation`  
          Флаг, включающий в рецепте требование ручных подтверждений выкладки в каждую локацию.
    * `activate_recipe`  
      Идентификатор рецепта в сервисе в Няне, используемого для активации сервиса.  
      При невалидном идентификаторе выкладка в Няне будет падать с ошибкой и перезапускаться, в надежде, что соответствующий рецепт всё-таки будет в сервис добавлен.  
      Кубик при этом будет продолжать ожидать конца выкладки, пока не упадёт по таймауту.  
      Нельзя использовать одновременно с настройкой `default_activate_recipe`.
    * `activate_prefix`  
      Необязательное поле, задающее префикс, используемый в рецепте для поиска задач активации локаций.  
      Работает только со специально подготовленными рецептами, поддерживающими полокационную выкладку с подтверждениями.  
      См. выше раздел [полокационных аппруфвов](#location-approves).
    * `patches`  
      Список патчей, указывающих какие ресурсы из релиза к каким ресурсам или слоям сервиса применять.  
      Не поддерживается одновременное использование вместе с полем `docker_image`.
    * `docker_image`  
      Полное имя докер-образа, включая tag, для применения к сервису.  
      Не поддерживается одновременное использование вместе с полем `patches`.

### common/nanny/set_snapshot_state

Кубик для смены состояния снапшота.

Используется, если в кубике `common/nanny/update_service` снапшот был создан в состоянии COMMITTED или PREPARED.

* `snapshots`  
  По умолчанию это поле автоматически конструируется из всех снапшотов, которые были созданы задачами `common/nanny/update_service` на предыдущих этапах flow.
* `approval_locations`  
  По умолчанию это поле автоматически конструируется из всех требующих одобрения локаций, которые были найдены задачами `common/nanny/update_service` и `common/nanny/set_snapshot_state` на предыдущих этапах flow.
* `config`  
* Все настройки кубика.
    * `target_status`  
      Состояние, в которое необходимо перевести снапшот.  
      Возможные варианты:  
          1. **COMMITTED** (сохранить изменения только в Няне, но ничего с ними не делать)  
          2. **PREPARED** (выкатить конфигурацию, но не переключать сервис на неё)  
          3. **ACTIVE** (выкатить конфигурацию и переключиться).  
    * `prepare_recipe`  
      Идентификатор рецепта в сервисе в Няне, которым осуществляется prepare конфигурации.  
      При отсутствии значения или указании невалидного идентификатора будет использоваться стандартный встроенный рецепт, который подходит большинству сервисов.
    * `default_activate_recipe`  
      Использовать стандартный рецепт для активации сервиса.  
      Нельзя использовать одновременно с настройкой `activate_recipe`.  
      При использовании стандартных рецептов настройка `activate_prefix` игнорируется.  
      По умолчанию используется стандартный рецепт **LOCATIONWISE**.
        * `recipe`  
          Тип стандартного рецепта. Поддерживается только значение **LOCATIONWISE**.
        * `manual_confirmation`  
          Флаг, включающий в рецепте требование ручных подтверждений выкладки в каждую локацию.
    * `activate_recipe`  
      Идентификатор рецепта в сервисе в Няне, используемого для активации сервиса.  
      При невалидном идентификаторе выкладка в Няне будет падать с ошибкой и перезапускаться, в надежде, что соответствующий рецепт всё-таки будет в сервис добавлен.  
      Кубик при этом будет продолжать ожидать конца выкладки, пока не упадёт по таймауту.  
      Нельзя использовать одновременно с настройкой `default_activate_recipe`.
    * `activate_prefix`  
      Необязательное поле, задающее префикс, используемый в рецепте для поиска задач активации локаций.  
      Работает только со специально подготовленными рецептами, поддерживающими полокационную выкладку с подтверждениями.  
      См. выше раздел [полокационных аппруфвов](#location-approves).
    * `snapshot`  
      Фильтр, используемый для поиска подходящего снапшота из всех, ранее созданных кубиками `common/nanny/update_service`.  
      Если снапшот создавался только один, это поле можно не заполнять, в противном случае в нём надо указать такой фильтр, чтобы под него попадал ровно один снапшот.
        * `nanny_installation`  
          Необязательное поле, инсталляция няни, использованная в снапшоте.
        * `service_id`  
          Необязательное поле, имя сервиса, использованное в снапшоте.
        * `snapshot_id`  
          Необязательное поле, идентификатор снапшота.  
          Это поле может быть нужно только в редких случаях, когда в рамках одного flow, сервис выкладывается несколько раз.  
          В таких ситуациях потребуется передавать это поле из output-параметров соответствующего кубика при помощи JMESPath.

### common/nanny/approve_location

Кубик для подтверждения выкладки снапшота в заданную локацию.

Может использоваться только со специально подготовленными рецептами, см. выше раздел [полокационных аппруфвов](#location-approves).

* `approval_locations`  
  По умолчанию это поле автоматически конструируется из всех требующих одобрения локаций, которые были найдены задачами `common/nanny/update_service` и `common/nanny/set_snapshot_state` на предыдущих этапах flow.
* `location_match`  
  Фильтр, используемый для поиска подходящей локации из всех, ранее созданных кубиками `common/nanny/update_service` и `common/nanny/set_snapshot_state`. 
  Должен быть составлен так, чтобы из всего списка совпала только одна локация.
    * `nanny_installation`
      Необязательное поле, инсталляция няни, использованная в выкладываемом снапшоте.
    * `service_id`  
      Необязательное поле, имя сервиса, использованное в выкладываемом снапшоте.
    * `cluster`  
      Необязательное поле, имя кластера, который требуется одобрить.  
      В редких случаях, когда выкладывается ровно один сервис с ровно одной локацией, это поле можно не заполнять.  
      Во flow, где выкладывается только один сервис из нескольих локаций, обычно достаточно ограничиться этим полем.

### Сноски {#recipe-diy}
¹ Если вам очень хочется написать свой рецепт активации для CI, приходите к нам в [RTCSUPPORT][rtcsupport].

[get-token]: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=34bd7c8cae1a417baa2d11bb0797a287
[ci-secret]: https://docs.yandex-team.ru/ci/secret
[yav]: https://yav.yandex-team.ru/
[docker-ya-package]: https://docs.yandex-team.ru/ya-make/usage/ya_package/docker
[nanny-patch-spec]: https://a.yandex-team.ru/arc_vcs/infra/nanny/nanny_tickets/nanny_tickets/tickets.proto?rev=ba69194d07bc1cd058313b3f01a2825b91c93f32#L115
[examples]: https://a.yandex-team.ru/arc_vcs/infra/nanny_ci/examples/
[docker-example]: https://a.yandex-team.ru/arc_vcs/infra/nanny_ci/examples/docker_no_approves/
[rtcsupport]: https://st.yandex-team.ru/RTCSUPPORT
[recipe]: https://nanny.yandex-team.ru/ui/#/alemate/recipes/_activate_service_configuration_locationwise_auto.yaml/
[approves-demo]: https://nanny.yandex-team.ru/ui/#/services/catalog/test_ci5_torkve/recipes
[example-set-snapshot]: https://a.yandex-team.ru/arc_vcs/infra/nanny_ci/examples/layer_manual_activate/a.yaml
[update-service-proto]: https://a.yandex-team.ru/arc_vcs/infra/nanny_ci/update_service/proto/update_service.proto
[set-snapshot-state-proto]: https://a.yandex-team.ru/arc_vcs/infra/nanny_ci/set_snapshot_state/proto/set_snapshot_state.proto
[approve-location-proto]: https://a.yandex-team.ru/arc_vcs/infra/nanny_ci/approve_location/proto/approve_location.proto

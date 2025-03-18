Ya.Deploy
=========

Тасклеты для релиза sandbox-ресурсов и docker-образов в Ya.Deploy.

***Внимание:***
* Eсли вы ранее пользовались тасклетом Релизной Машины `common/deploy/release`, требующим настройки релизной интеграции в стейдже, то имеет смысл удалить старое релизное правило, перенеся список патчей из правила непосредственно в настройки кубика.
* Любые релизные правила правила (Rule) из Deploy нужно удалить. Оставляйте их только в том случае, если вы уверены в их совместимости с текущими параметрами деплоя.


При обнаружении проблем с данными тасклетами обращайтесь в [RTCSUPPORT][rtcsupport].

***NOTE:*** этот файл описывает новые тасклеты от команды Деплоя. Документация на старые тасклеты лежит в файле `README.deprecated.md`.

- [Получите токен][get-token]
- Положите с ключом `deploy_ci.token` в [секрет CI][ci-secret] в [Секретнице][yav]
- Проверьте, что у робота есть права `Maintainer` в Ya.Deploy, если нет, их необходимо выдать
- Добавьте во flow запуск `common/deploy/create_release`
    - Сконфигурируйте кубик, указав `stage_id` и список патчей, описывающих, какими ресурсами патчить какие части стейджа.  
      Структура патчей соответствует proto-сообщению [TDeployPatchSpec][deploy-patch-spec] из релизной интеграции Ya.Deploy. [Примеры патчей](#patches-examples) смотрите ниже.

## Примеры конфигураций

Полные примеры можно посмотреть [в аркадии][examples]. Ниже приведены выдержки из этих примеров, демонстрирующие непосредственно флоу выкладки.

### Сборка и выкладка нового статического ресурса

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
          targets: infra/deploy_ci/examples/pipeline_without_approves
          arts: infra/deploy_ci/examples/pipeline_without_approves/http_server
          result_rt: TEST_RESOURCE
          result_single_file: true
          build_type: release
      # Кубик для выкладки собранных артефактов в деплой.
      release-to-deploy:
        task: common/deploy/create_release
        title: Deploy
        needs: build
        stage: deploy
        input:
          config:
            # Имя стейджа, который мы обновляем
            stage_id: deploy-ci-test
            # Список применяемых патчей. В данном случае мы собираем и обновляем только один ресурс.
            patches:
              - sandbox:
                  # Ищем собранный артефакт типа TEST_RESOURCE
                  sandbox_resource_type: TEST_RESOURCE
                  # и обновляем им статический ресурс resource1 в деплой юните du2
                  static:
                    deploy_unit_id: du2
                    static_resource_ref: resource1
```

### Сборка и выкладка статического ресурса с простыми полокационными аппрувами

Для работы локационных аппрувов необходимо настроить свой стейдж в соответствии с [инструкцией][per-location].

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
          targets: infra/deploy_ci/examples/simple_pipeline
          arts: infra/deploy_ci/examples/simple_pipeline/http_server
          result_rt: TEST_RESOURCE
          result_single_file: true
          build_type: release
      # Кубик для выкладки собранных артефактов в деплой.
      release-to-deploy:
        task: common/deploy/create_release
        title: Deploy
        needs: build
        stage: deploy
        input:
          config:
            # Имя стейджа, который мы обновляем
            stage_id: deploy-ci-test
            # Список применяемых патчей. В данном случае мы собираем и обновляем только один ресурс.
            patches:
              - sandbox:
                  # Ищем собранный артефакт типа TEST_RESOURCE
                  sandbox_resource_type: TEST_RESOURCE
                  # и обновляем им статический ресурс resource1 в деплой юните du1
                  static:
                    deploy_unit_id: du1
                    static_resource_ref: resource1
      # Кубик одобрения локационной выкладки. В этом примере мы используем простую автогенерацию,
      # когда нет необходимости вручную прописывать во флоу все локации — количество экземпляров кубика
      # создаётся автоматически на основании информации о стейдже. Недостатком этого подхода является то,
      # что кубики аппрува равнозначны и пользователь может одобрять выкатку локаций в произвольном порядке.
      approve-location:
        task: common/deploy/approve_location
        needs: release-to-deploy
        stage: deploy
        multiply:
          # Здесь мы указываем ID предыдущего кубика release-to-deploy, который в output-артефактах
          # создаёт список approval_locations, содержащий информацию о том, какие локации требуют аппрува.
          by: ${tasks.release-to-deploy.approval_locations}
          title: "Approving location ${by.cluster} for deploy unit ${by.deploy_unit}"
        input:
          location_match: ${by}
        # Требуем, чтобы пользователь явно одобрил выкатку в интерфейсе CI.
        manual:
          enabled: true
          prompt: "Approve location ${by.cluster} for deploy unit ${by.deploy_unit}?"
```

### Сборка и выкладка статического ресурса со строгим порядком аппрува локаций

Для работы локационных аппрувов необходимо настроить свой стейдж в соответствии с [инструкцией][per-location].

```yaml
flows:
  autobuild:
    jobs:
      # Кубик (или кубики), описывающий сборку.
      # На выходе получается произвольное множество объектов SandboxResource,
      # которые могут использоваться на стадии выкладки.
      build:
        task: common/arcadia/ya_make
        title: Autobuild
        stage: build
        input:
          targets: infra/deploy_ci/examples/sequential_approves
          arts: infra/deploy_ci/examples/sequential_approves/http_server
          result_rt: TEST_RESOURCE
          result_single_file: true
          build_type: release
      # Кубик для выкладки собранных артефактов в деплой.
      release-to-deploy:
        task: common/deploy/create_release
        title: Create release
        needs: build
        stage: deploy
        input:
          config:
            # Имя стейджа, который мы обновляем
            stage_id: deploy-ci-test
            # Список применяемых патчей. В данном случае мы собираем и обновляем только один ресурс.
            patches:
              - sandbox:
                  # Ищем собранный артефакт типа TEST_RESOURCE
                  sandbox_resource_type: TEST_RESOURCE
                  # и обновляем им статический ресурс resource1 в деплой юните du1
                  static:
                    deploy_unit_id: du1
                    static_resource_ref: resource1
      # Кубик одобрения выкатки в ДЦ man
      approve-location-man:
        task: common/deploy/approve_location
        needs: release-to-deploy
        stage: deploy
        input:
          # явно указываем во входных параметрах кластер
          location_match:
            cluster: man
        title: Approve location man
        manual:
          enabled: true
          prompt: "Approve location man?"
      # Ваш опциональный кубик, в котором можно запустить какие-то тесты для проверки, что релиз успешно выехал в man
      run-tests-man:
        task: dummy
        title: Run tests in man location
        needs: approve-location-man
      # Кубик одобрения выкатки в ДЦ sas
      approve-location-sas:
        task: common/deploy/approve_location
        needs: run-tests-man
        stage: deploy
        input:
          # явно указываем во входных параметрах кластер
          location_match:
            cluster: sas
        title: Approve location sas
        manual:
          enabled: true
          prompt: "Approve location sas?"
      # Ваш опциональный кубик, в котором можно запустить какие-то тесты для проверки, что релиз успешно выехал в sas
      run-tests-sas:
        task: dummy
        title: Run tests in sas location
        needs: approve-location-sas
```

### Сборка и выкладка docker-образа с помощью ya package

Для сборки docker-образа при помощи задачи YA_PACKAGE необходимо предварительно подготовить Dockerfile и json-спецификацию сборки [по инструкции][docker-ya-package].
Пример Dockerfile и файла build.json можно посмотреть [в примерах][docker-example].

Кроме того в соответствии с [требованиями Ya.Deploy][docker-ya-deploy] необходимо выдать роботу **robot-qloud-client** роль **viewer** на ваш docker-репозиторий в registry.

Для docker необходимо указывать при сборке и выкладке имя тега (как правило для него используется версия), поэтому здесь приводится пример со сборкой Docker-образа в релизном flow, где доступно поле `${context.version_info}`:

```yaml
  flows:
    autobuild:
      title: Autodeploy deploy_ci docker pipeline commit
      jobs:
        build:
          task: common/arcadia/ya_package_2
          title: Build docker
          stage: build
          input:
            # Путь относительно корня аркадии к спеке собираемого с помощью ya package пакета
            packages: infra/deploy_ci/examples/docker_pipeline/build.json
            package_type: docker
            # Имя docker-репозитория. Это не имя docker-образа: если вы заливаете свой
            # пакет в докер, например, как deploy-ci/http_server_example:1.0
            # то "deploy-ci" — это репозиторий, "http_server_example" — имя образа,
            # а "1.0" — имя тега.
            docker_image_repository: deploy-ci
            # Имя робота, который авторизуется в Registry
            docker_user: robot-drug-deploy
            # Имя хранилища Sandbox Vault, содержащего OAuth-токен для доступа docker_user к registry.
            # В качестве токена в Vault можно сохранить тот же токен, который вы сохранили
            # под именем deploy_ci.token в yav. 
            docker_token_vault_name: docker.registry.token
            docker_push_image: true
            # Явно задаём имя тега, который мы собираем. Оно потребуется ниже.
            custom_version: "${context.version_info.full}"
        release-to-deploy:
          task: common/deploy/create_release
          title: Create release
          needs: build
          stage: deploy
          input:
            config:
              stage_id: deploy-ci-test
              patches:
                - docker:
                    docker_image_ref:
                        # Имя deploy unit, в котором производятся изменения.
                        deploy_unit_id: du3
                        # Имя обновляемого box в этом deploy unit.
                        box_id: box
                    # Полное имя применяемого docker-образа, включая
                    # имя тега (совпадает с тем тегом, который мы собрали выше).
                    image_name: "deploy-ci/example_http_server:${not_null(context.rollback_to_version.full, context.version_info.full)}"
```

## Примеры патчей {#patches-examples}

Каждый приведённый патч указывается в качестве элемента массива `patches` во входных параметрах кубика `common/deploy/create_release`. Массив обязан содержать хотя бы один патч.  
Для всех возможных полей патчей рекомендуется смотреть структуру proto-сообщения [TDeployPatchSpec][deploy-patch-spec].

### Обновление статического ресурса

```yaml
sandbox:
  # Тип sandbox-ресурса, который необходимо найти в собранных ранее во flow артефактах.
  sandbox_resource_type: TEST_RESOURCE
  # Опциональный словарь требуемых атрибутов ресурса, который также используется для поисков правильного ресурса
  # в собранных артефактах. Как правило имеет смысл, если вы собираете в рамках флоу несколько артефактов
  # с одинаковым типом.
  attributes:
    key1: value1
    key2: value2
  static:
    # Имя deploy unit, в котором производятся изменения.
    deploy_unit_id: du1
    # Имя статического ресурса в этом deploy unit.
    static_resource_ref: resource1
```

### Обновление слоя

```yaml
sandbox:
  # Тип sandbox-ресурса, который необходимо найти в собранных ранее во flow артефактах.
  sandbox_resource_type: TEST_RESOURCE
  # Опциональный словарь требуемых атрибутов ресурса, который также используется для поисков правильного ресурса
  # в собранных артефактах. Как правило имеет смысл, если вы собираете в рамках флоу несколько артефактов
  # с одинаковым типом.
  attributes:
    key1: value1
    key2: value2
  static:
    # ID deploy unit, в котором производятся изменения.
    deploy_unit_id: du1
    # ID слоя в этом deploy unit.
    layer_ref: layer1
```

### Обновление динамического ресурса

```yaml
sandbox:
  # Тип sandbox-ресурса, который необходимо найти в собранных ранее во flow артефактах.
  sandbox_resource_type: TEST_RESOURCE
  # Опциональный словарь требуемых атрибутов ресурса, который также используется для поисков правильного ресурса
  # в собранных артефактах. Как правило имеет смысл, если вы собираете в рамках флоу несколько артефактов
  # с одинаковым типом.
  attributes:
    key1: value1
    key2: value2
  dynamic:
    # ID обновляемого динамического ресурса в стейдже
    dynamic_resource_id: dr1
    # Идентификатор группы подов в динамическом ресурсе. В подавляющем большинстве случаев
    # динамические ресурсы содержат всего одну группу с идентификатором "all"
    deploy_group_mark: all
```

### Обновление докер-образа

```yaml
docker:
  docker_image_ref:
    # Имя deploy unit, в котором производятся изменения.
    deploy_unit_id: du1
    # Имя бокса в этом deploy unit.
    box_id: box1
  # Имя образа, который необходимо выложить. Обратите внимание, что в отличие от sandbox-ресурсов,
  # для docker-образов пока не существует установленного способа описать докер образ в качестве выходного
  # артефакта какого-то из кубиков на стадии сборки, поэтому в отличие от релизной интеграции Ya.Deploy
  # здесь указывется имя образа вместе с конкретным тегом.
  # В данном примере мы предполагаем, что данный патч используется в релизном флоу, где в качестве имени
  # тега будет использоваться одно из значений ${not_null(context.rollback_to_version.full, context.version_info.full)},
  # поддерживая работу flow как в обычном режиме, так и в режиме rollback-а.
  # Сам образ должен быть собран на стадии сборки и запушен в registry.yandex.net под этим же тегом.
  image_name: "myproject/myimage:${not_null(context.rollback_to_version.full, context.version_info.full)}"
```

[get-token]: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=c185b75fce81443c86750dd411b8943a
[ci-secret]: https://docs.yandex-team.ru/ci/secret
[yav]: https://yav.yandex-team.ru/
[per-location]: https://deploy.yandex-team.ru/docs/concepts/perlocation/perlocation20
[docker-ya-package]: https://docs.yandex-team.ru/ya-make/usage/ya_package/docker
[docker-ya-deploy]: https://deploy.yandex-team.ru/docs/concepts/stage/ispolzovanie-docker-obraza
[deploy-patch-spec]: https://a.yandex-team.ru/arc_vcs/yp/yp_proto/yp/client/api/proto/deploy_patch.proto?rev=ed71cce3ce6220e39689ebefee8c9a7e6a731b41#L120
[examples]: https://a.yandex-team.ru/arc_vcs/infra/deploy_ci/examples/
[docker-example]: https://a.yandex-team.ru/arc_vcs/infra/deploy_ci/examples/docker_pipeline/
[rtcsupport]: https://st.yandex-team.ru/RTCSUPPORT

Ya.Deploy
=========

Тасклет для релиза sandbox-ресурсов и docker-образов в Ya.Deploy

- [Получите токен][get-token]
- Положите с ключом `simple-releaser.release.token` в [секрет CI](https://docs.yandex-team.ru/ci/secret) в [Секретнице](https://yav.yandex-team.ru/)
- Проверьте, что у робота есть права `Maintainer` в Ya.Deploy, если нет, их необходимо выдать.
- Добавьте в тасклет поддержку вашего ресурса (`актуально только для локальных запусков`):
    - Добавьте PEERDIR c вашим типом Sandbox ресурса в [реализацию тасклета][deploy-tasklet-impl]. \
    [Пример ресурса CI_API_APP][ci_api_app] \
    [Пример импорта][import-resource]
    - Добавьте импорт ресурса в [исполняемый файл][python-import], с комментарием `# noqa`
    - Соберите тасклет и загрузите бинарник тасклета:
    ```shell
    ~/arc/arcadia/release_machine/tasklets/release_to_ya_deploy $ ya m
    ~/arc/arcadia/release_machine/tasklets/release_to_ya_deploy $ ./release_to_ya_deploy sb-upload --sb-schema
    ```
    - Обновите id ресурса [в реестре][registry]
- Добавьте запуск `common/deploy/release`
- Сконфигурируйте job, добавив в `input` необходимые поля из [Config][ya-deploy-config].
  По-умолчанию, тасклет будет пытаться ждать завершения выкатки ресурсов на стейдж с таймаутом в 1 час.
  Если за это время сервис не выкатится - тасклет завершится с ошибкой. Примеры конфигураций см. ниже.
- Добавьте релизное правило для вашего стейджа [по инструкции](https://wiki.yandex-team.ru/deploy/docs/concepts/release-integration/), отключив при этом автокоммит тикетов. Автокоммит не нужен, так как тасклет по-умолчанию делает это сам (если не указано do_not_commit_release_tickets). Иначе возможна попытка деплоя даже при возникновении проблем в тасклете (красный кубик в интерфейсе CI).

***ВАЖНО!*** Необходимо создать релизное правило *до* релиза. Иначе [релизный тикет не создастся и флоу упадет](https://st.yandex-team.ru/RTCSUPPORT-8944#602f9dc3f9063e5600fa4a8e)

**Примеры кофигураций**
  1. Релиз ресурса sandbox в stable
  ```yaml
    config:
      release_artifact_type: SANDBOX_RESOURCE  # тип артефакта (по-умолчанию, SANDBOX_RESOURCE, так что можно не укаывать)
      sandbox_resource_type: CI_API_APP  # тип ресурса с приложением
      release_stage: stable  # тип релиза, согласно релизному правилу
      release_subject:  "Some subject" # название релизного тикета
      release_notes: Release from new ci  # комментарий для релизного тикета
      stage_name: your-deploy-stage-name  # имя стейджа в ya.deploy
      do_not_wait_for_deploy: False  # если True, тасклет не будет ждать окончания выкатки
      do_not_commit_release_tickets: False  # если True, тасклет не будет коммитить релизные тикеты сам (не рекомендуется использовать без необходимости)
      wait_for_deploy_timeout_minutes: 20  # [default: 60] сколько минут ждать выкатки в сервис (потом падает)
  ```
  2. Релиз docker образа в testing
  ```yaml
    config:
      release_artifact_type: DOCKER_IMAGE  # тип артефакта (для релиза docker образа необходимо указывать явно)
      sandbox_resource_type: CI_API_APP  # тип ресурса с образом (да, он должен быть, и там должен быть атрибут resource_version с указанием версии, например registry.yandex.net/load/tank-finder:0.4.13)
      release_stage: testing  # тип релиза, согласно релизному правилу
      release_subject:  "Some subject" # название релизного тикета
      release_notes: Release from new ci  # комментарий для релизного тикета
      stage_name: your-deploy-stage-name  # имя стейджа в ya.deploy
  ```

**Важно:**
- Если вы используете `YA_PACKAGE` в качестве таска сборки, **не выставляйте**, пожалуйста, параметр `release_to_ya_deploy = True`.
  Это приведет к дублированию релизных тикетов в системе деплоя.

[ci_api_app]: https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/ci/__init__.py?rev=6741273#L5
[deploy-tasklet-impl]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_ya_deploy/impl/ya.make
[get-token]: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=23b4f83306e3469abdee07054d307e7c
[import-resource]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_ya_deploy/impl/ya.make?rev=7172231#L8
[python-import]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_ya_deploy/impl/__init__.py?rev=7172231#L2
[registry]: https://a.yandex-team.ru/arc/trunk/arcadia/ci/registry/common/deploy/release.yaml
[ya-deploy-config]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_ya_deploy/proto/release_to_ya_deploy.proto

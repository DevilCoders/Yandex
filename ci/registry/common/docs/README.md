Yandex Docs
===========

Тасклеты для сборки и релиза документации https://docs.yandex-team.ru/
Справка: https://docs.yandex-team.ru/docstools
Подробная документация по настройке: https://docs.yandex-team.ru/docstools/deploy


Deploy
------

Собирает документацию и загружает её в тестовый контур.
Если передать arcanum_review_id, то оставляет ссылку на выложенную документацию в комментарии пулл-реквеста.

```yaml
actions:
  # определение экшена, который будет вызван при пулл-реквесте,
  # затрагивающим вашу документацию
  docs:
    title: Deploy docs
    flow: deploy-pr-docs
    triggers:
      - on: pr

flows:
  deploy-pr-docs:
    jobs:
      deploy:
        title: Deploy docs
        task: common/docs/deploy
        input:
          targets: path/to/your/projects # путь до вашего проекта от корня Аркадии
          arcanum_review_id: ${context.launch_pull_request_info.pull_request.id}
```

Release
-------

Релизит собранную и загруженную ранее документацию в [тестовую](https://testing.docs.yandex-team.ru/) или продакшен 
среду.

```yaml
releases:
  release-docs:
    title: Release docs
    flow: release-docs
    auto: true
    stages:
      - id: build
        title: Build
        displace: true
      - id: testing
        title: Testing
        displace: true
      - id: stable
        title: Stable

release-docs:
  jobs:
    deploy:
        title: Deploy docs
        task: common/docs/deploy
        stage: build
        input:
          targets: path/to/your/project # путь до вашего проекта от корня Аркадии

    release-to-testing:
      title: Release docs to testing
      task: common/docs/release
      needs: deploy
      stage: testing
      input:
        projects: ${tasks.deploy.output_params.projects}
        environments: testing

    release-to-stable:
      title: Release docs to stable
      task: common/docs/release
      needs: release-to-testing
      manual: true
      stage: stable
      input:
        projects: ${tasks.deploy.output_params.projects}
        environments: production

```

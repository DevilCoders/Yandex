Releasers
===============

Для того, чтобы воспользоваться любым из перечисленных тасклетов, необходимо вначале сделать следующее:

- [Получите токен][get-token]
- Положите с ключом `simple-releaser.release.token` в [секрет CI](https://docs.yandex-team.ru/ci/secret) в [Секретнице](https://yav.yandex-team.ru/)

# Simple Releaser

Общий тасклет. Релизит ресурсы в любую из доступных систем деплоя. Имеет громоздкий конфиг, поэтому для удобства сделаны отдельные тасклеты для релизов в каждую из систем деплоя с более простыми конфигами.
- Добавьте запуск `common/releases/simple_releaser`
- Сконфигурируйте джобу, добавив `input` в описании джобы необходимые поля из [Config][simple-releaser-config]. Для этого можно воспользоваться примерами [отсюда](https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/simple_releaser)
- Добавьте релизное правило для вашего стейджа [по инструкции](https://wiki.yandex-team.ru/deploy/docs/concepts/release-integration/)


# Release to sandbox

Релизит ресурсы в sandbox (то есть, нажимает в таске сборки кнопку `Release`). Под капотом все тот же `simple_releaser`.
- Добавьте запуск `common/releases/release_to_sandbox`
- Сконфигурируйте джобу, добавив `input` в описании джобы необходимые поля из [Config][release-to-sandbox-config]. Для этого можно воспользоваться [примером][release-to-sandbox-input-ex]


# Release rm component

Тасклет-зеркало для таска RELEASE_RM_COMPONENT_2. Умеет почти все, что и исходный таск. Под капотом все тот же `simple_releaser`.
Тасклет будет использоваться для перехода RM из TestEnv в CI.
- Добавьте запуск `common/releases/release_rm_component`
- Сконфигурируйте джобу, добавив `input` в описании джобы необходимые поля из [Config][release-rm-component-config]. Для этого можно воспользоваться [примером][release-rm-component-input-ex]


[get-token]: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=23b4f83306e3469abdee07054d307e7c
[simple-releaser-config]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/simple_releaser/proto/simple_releaser.proto
[release-to-sandbox-config]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_sandbox/proto/release_to_sandbox.proto
[release-to-sandbox-input-ex]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_sandbox/input.example.json
[release-rm-component-config]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_rm_component/proto/release_rm_component.proto
[release-rm-component-input-ex]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_rm_component/input.example.json

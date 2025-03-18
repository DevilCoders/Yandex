# common/nanny/release

- [Получите токен][get-token]
- Положите с ключом `simple-releaser.release.token` в [секрет CI](https://docs.yandex-team.ru/ci/secret) в [Секретнице](https://yav.yandex-team.ru/)
- Собирайте свой пакет с помощью **common/nanny/kosher_ya_make**


Релизит ресурсы в nanny.
По факту, нажимает в таске сборки кнопку `Release`, и если таск сборки отнаследован от ReleaseToNannyTask или ReleaseToNannyTask2, он запустит процесс релиза в няню.
Умеет дожидаться закрытия релизного тикета (что обычно соответствует окончанию деплоя на сервисы).
Под капотом все тот же `simple_releaser`.
- Добавьте запуск `common/nanny/release`
- Сконфигурируйте джобу, добавив `input` в описании джобы необходимые поля из [Config][release-to-nanny-config]. Для этого можно воспользоваться [примером](https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_nanny/input.example.json)


[get-token]: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=23b4f83306e3469abdee07054d307e7c
[release-to-nanny-config]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_nanny/proto/release_to_nanny.proto


# common/nanny/kosher_ya_make
Специальная версия ya make, которая нужна при использовании кубика `common/nanny/release`

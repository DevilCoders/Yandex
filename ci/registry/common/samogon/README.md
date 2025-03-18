Samogon
=======

Релизит ресурс в самогоне

- [Получите токен][get-token] в Samogon
- Положите с ключом `samogon.token` в [секрет CI](https://docs.yandex-team.ru/ci/secret) в [Секретнице](https://yav.yandex-team.ru/)
- Добавьте запуск `common/samogon/release`.
- Сконфигурируйте джобу, добавив `input` в описании джобы необходимые поля
  из [Config][samogon-config]

Пример конфигурации можно посмотреть [здесь][samogon-config-example]

[samogon-config]: https://a.yandex-team.ru/arc/trunk/arcadia/release_machine/tasklets/release_to_samogon/proto/release_to_samogon.proto?rev=7149649#L15
[samogon-config-example]: https://a.yandex-team.ru/arc/trunk/arcadia/testenv/ci/backend/a.yaml?rev=7080104#L31
[get-token]: https://oauth.yandex-team.ru/authorize?response_type=token&client_id=23b4f83306e3469abdee07054d307e7c

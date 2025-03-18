Приложение живет в Yandex Deploy.
- Тестинг: https://deploy.yandex-team.ru/stages/ci-ayamler-api-testing
- Прод: https://deploy.yandex-team.ru/stages/ci-ayamler-api-stable

Вручную собрать пакет можно командой:
```
ya package --target-platform=linux --upload --upload-resource-attr=ttl=inf package.json
```

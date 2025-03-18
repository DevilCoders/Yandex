Event reader
=======

Для локального запуска надо:
1. Создать consumer в LB со своим логином https://lb.yandex-team.ru/lbkx/accounts/ci/consumers/dev?page=browser&type=directory
2. Получить oAuth токен http://oauth.yandex-team.ru/authorize?response_type=token&client_id=11515c5e5e994dfe8196ccfd6eb42dd8
3. Прописать токен в .ci/ci-local.properties в проперти ci.event-reader.logbroker.oauth-token

JFI. При локальном запуске event-reader обрабатывает только PR'ы запустившего.
Опция: `ci.arcanumEventService.authorFilter=${user.name}`

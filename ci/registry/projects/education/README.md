# Тасклеты Образования

## Deploy Release - Тасклет для Релизов с ожиданием 

### Описание

Тасклет для релизов в деплой. Проводит обновление стейджа и ждет завершение выкатки новой ревизии.
Умеет работать с глобальными опциями по всем ворклоадам, боксам и деплой юнитам, так и с отдельными опциями.

### Почему не подошел `common/deploy/create_release`

- Работает через деплой релизных тикетов и требует удаления всех релизных правил. _Мы хотели иметь релизное правило на обновление докера по крону, и на этом тасклет превращался в тыкву для нас_
- Умеет в обновление ресурсов, слоев, и докера, но не умеет в обновление переменных окружения. _Для ускорения выкатки релиза нам не хотелось бы каждый раз пересобирать все слои приложения, когда просто нужно обновить бинарь_


### Как использовать?

1. Добавить в ваш a.yaml задачу `projects/education/deploy_release`
2. Описать `s_body`
3. Добавить в ваш [секрет](https://docs.yandex-team.ru/ci/basics#sections) (`ci.secret`) `DCTL_TOKEN` от вашего робота, значение можно найти по [ссылке](https://wiki.yandex-team.ru/e7n/devold/most-usable-oauth-tokens/). Ключ записать в поле `input.config.dctl_token_key`

Должно получится, как по [примеру](https://arcanum.yandex-team.ru/arc_vcs/education/services/enigma/a.yaml?rev=4af2038a2e1a72f760901bc50e98e5a0e089dde0#L192)



### Где почитать про входные значения для `s_body`?

Формат json-строки соответствует ключам `globals`, `deploy_units`, `comment` [документации](https://docs.yandex-team.ru/edu-enigma/release_handler#format-zaprosa-reliznoj-ruchki) релизной ручки Инфраструктурного сервиса "Энигма". Ожидание завершения выкладки включено по умолчанию.

#### Примеры различных `s_body`

##### [Модификация переменных окружения](https://docs.yandex-team.ru/edu-enigma/release_handler#operacii-s-peremennymi-okruzheniya)

##### [Модификация докера](https://docs.yandex-team.ru/edu-enigma/release_handler#operacii-s-docker-obrazom)

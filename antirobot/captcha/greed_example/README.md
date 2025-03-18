Предполагается, что nodejs и npm уже установлены

Создаем файл ~/.npmrc с содержимым
```
registry = http://npm.yandex-team.ru/
```

```
cd ~/arcadia/antirobot/captcha/greed_example
```


Устанавливаем зависимости:
```
npm i
```

Запускаем сборку:
```
npm run build
```


Мы сгенерировали файлики в папке `~/arcadia/antirobot/captcha/greed_example/server/generated`

`greed.js` - сама библиотека

`mapping.json` - маппинг читаемых имен фичей в обфусцированные

осталось собрать и запустить веб-сервер

```
cd server
ya make . -r && ./server
```

В тестовом сервисе реализованы 2 варианта отправки данных: без обфускации данных (`http://localhost:12312/`), и с обфускацией (`http://localhost:12312/hard`)


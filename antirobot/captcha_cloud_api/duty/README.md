## Обновление JSON
Дока: https://wiki.yandex-team.ru/cloud/devel/gore/

Команда для обновления:
```sh
curl -kv -XPATCH https://resps-api.cloud.yandex.net/api/v0/services/smartcaptcha  -T duty.json -H "Authorization: OAuth ..."
```

-----
Вопросы о боте можно задавать в чате https://t.me/joinchat/TunREb2h4a0SRWN8
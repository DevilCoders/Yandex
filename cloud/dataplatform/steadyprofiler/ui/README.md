
# Data UI Core App Template

[![oko health](https://badger.yandex-team.ru/oko/repo/data-ui/ui-core-template/health.svg)](https://oko.yandex-team.ru/repo/data-ui/ui-core-template)

Шаблон приложения на [UICore]. Доступен также [шаблон API-only приложения][CoreTemplate].


## Быстрый старт

1. Удалите историю шаблона и инициализируйте репозиторий: `rm -rf .git && git init`
2. Замените `profiler-archive` на имя вашего приложения в `package.json` и `src/server/index.ts`. Если вы собираетесь деплоить приложение в qloud, обновите также значения в секции `deploy`
3. Установите зависимости: `npm ci`
4. Запустите приложение: `npm run dev`

Пример конфига для nginx (предполагается, что дев-сервер был настроен [по инструкции][devsetup] и ssl-сертификат уже есть):

```
server {
    server_name tserakhau.sas.yp-c.yandex.net;

    include common/ssl;
    include common/gzip;

    root /home/tserakhau/go/src/a.yandex-team.ru/transfer_manager/go/internal/profiler/ui/dist/public;

    location /api/ {
        rewrite ^/api/?(.*)$ /$1 break;    
        proxy_pass  http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    }

    location / {
        try_files $uri @server;
    }

    location /build/ {
        try_files $uri @client;
    }

    location @server {
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header Host $host;
        proxy_pass http://unix://home/tserakhau/go/src/a.yandex-team.ru/transfer_manager/go/internal/profiler/ui/dist/run/server.sock;
        proxy_redirect off;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }

    location @client {
        proxy_pass http://unix://home/tserakhau/go/src/a.yandex-team.ru/transfer_manager/go/internal/profiler/ui/dist/run/client.sock;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Request-ID $request_id;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_redirect off;
    }
}
```

Больше информации по настройке и эксплуатации Core доступно в нашей документации:

- Core (общие практики): https://ui.yandex-team.ru/
- UICore (особенности разработки, настройка CDN): https://github.yandex-team.ru/data-ui/ui-core


[Core]: https://github.yandex-team.ru/data-ui/core
[UICore]: https://github.yandex-team.ru/data-ui/core
[CoreTemplate]: https://github.yandex-team.ru/data-ui/core-template/
[devsetup]: https://ui.yandex-team.ru/dev-server#zakaz-sertifikatov

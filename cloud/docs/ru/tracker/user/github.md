# Системы контроля версий

## GitHub {#section_nb1_3mx_3bb}

Чтобы привязывать тикеты, пулл-реквесты и новые ветки вашего репозитороия к задачам в {{ tracker-name }}:

1. Настройте в репозитории вебхук с параметрами:
    - **Payload URL** — `https://st-api.yandex-team.ru/v2/system/github/receive`.
    - **Content type** — `application/json`.
    - **Events** — `Pull Request` или `Issues` или `Branch or tag creation`.
1. В названии ветки или описании пулл-реквеста или тикета укажите ключ задачи, с которой нужно создать связь.
    
    {% note warning %}

    {{ tracker-name }} не обработает ключ задачи, если перед ним указаны символы `#` или `~`.

    {% endnote %}

Привязанные тикеты, пулл-реквесты и ветки отображаются на странице задачи в блоке связей.

## Mercurial {#section_jqk_zqx_3bb}

Чтобы привязать коммиты к задачам в {{ tracker-name }}, укажите в описании коммита ключ задачи. Привязанные коммиты отображаются на странице задачи на вкладке **Коммиты**.
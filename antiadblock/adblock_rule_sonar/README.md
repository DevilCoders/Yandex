# ADBlock Rule Sonar

![](https://jing.yandex-team.ru/files/lawyard/Novyi_risunok_-_Google_Risunki_2017-03-09_18-28-38.png)

Утилита для парсинга правил из AdBlock листов.

Правила хранятся в таблицах на yt:
* [Правила, действующие на конкретных партнеров](https://yt.yandex-team.ru/hahn/#page=navigation&path=//home/antiadb/sonar/sonar_rules&offsetMode=key)
* [Общие правила](https://yt.yandex-team.ru/hahn/#page=navigation&path=//home/antiadb/sonar/general_rules&offsetMode=key)
* [Ссылки и метаинформация о списках правил](https://yt.yandex-team.ru/hahn/navigation?offsetMode=key&path=//home/antiadb/sonar/adblock_sources)

## Использование

### Использование из командной строки
При запуске утилиты можно использовать дополнительные команды, справку по которым видно при запуске `--help`:
```bash
ya make --checkout
./sonar/adblock_rule_sonar --help

DEBUG [2019-03-11 15:45:34,526]  Loading sonar config
usage: adblock_rule_sonar [-h] [--yt_token TOKEN] [--send_rules]
                          [--domain DOMAIN] [--list LIST]

AdBlock Sonar. Tool for adblock rule parsing.

optional arguments:
  -h, --help        show this help message and exit
  --yt_token TOKEN  Using yt for storing new rules and getting current active.
                    Expects oAuth token for YT as value
  --send_rules      If this enables sonar will send rules to yt. USE ONLY FOR
                    SANDBOX RUNS
  --create_tickets  If this enables sonar will create tickets in StarTrack
                    about cookie-remover rules. USE ONLY FOR SANDBOX RUNS
  --domain DOMAIN   This option allows to search rules for single domain.
                    Expects domain name or regexp
  --list LIST       This option allows to search rules in single list. Expects
                    list url

```

### Регулярный запуск
Sandbox таска [AAB_RUN_SONAR_BIN](https://sandbox.yandex-team.ru/tasks?page=1&pageCapacity=20&forPage=tasks&type=AAB_RUN_SONAR_BIN),
умеет запускать собранную и загруженную как Sandbox-ресурс `ADBLOCK_RULE_SONAR_BIN`, утилиту.
Таска всегда использует последнюю версию `ADBLOCK_RULE_SONAR_BIN`, которая обновляется при коммитах в папку `antiadblock/adblock_rule_sonar`.
Для регулярного запуска `AAB_RUN_SONAR_BIN` (раз в час) используется [sheduler](https://sandbox.yandex-team.ru/scheduler/13662).
B параметрах `Email receiver` можно указать (через запятую) список тех, кто будет получать письма о новых правилах,
а в `adblock_rule_sonar binary` нужную версию загруженного `adblock_rule_sonar`.
По умолчанию на получение писем подписан robot-antiadb@yandex-team.ru и рассылка antiadb-sonar@yandex-team.ru.

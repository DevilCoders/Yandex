# AntiAdblock Detect Checker

Скрипт проверки детекта на сайте.
сейчас точно работает проверка в:
- Браузерах: Yandex Browser, Chrome, Firefox
- Расширениями: Adguard, Adblock, Adblock+, Adblock+ developer version, Ghostery, Ublock(только Chrome и YaBro), Ublock Origin, без расширения и Режим Инкогнито (проверка отсутствия ложного детекта)

Используемая сейчас квота - [selenium](https://selenium.yandex-team.ru/#quota/selenium)

[Подробнее про наш Selenium grid](https://wiki.yandex-team.ru/selenium/)

[Графики нагрузки на Selenium grid](https://grafana.qatools.yandex-team.ru/d/0zPa-K6kz/selenium-tv?refresh=1m&panelId=5&fullscreen&orgId=1&from=now-3h&to=now)

Для запуска просто запустить `check_detect.py`.

Сайты для проверки (**SITES_TO_CHECK**) хранятся в `config.py` в следующем формате: `[pid скрипта детекта, cайт, кука детекта]` где `cайт`-полный путь к странице сайта этого партнера, где работает детект адблока.

При запуске можно передать ключи:
- `--help` - выдает полный список команд с возможными вариантами значений
- `--local_run` - локальный запуск, не будет писать результаты в stat
- `--pids <значение>` - service_id партнера для проверки только его, а не всех партнеров. Можно задавать список через запятую, пробелов внутри списка быть не должно. Удобно для локального запуска, партнера(ов) надо заранее добавить в config
- `--browsers <значение>` - список браузеров в котором производить проверку, через запятую без пробелов. По умолчанию выполняется во всех
- `--adblocks <значение>` - adblock расширения с которыми производить проверку. Можно задавать список через запятую, пробелов внутри списка быть не должно. По умолчанию выполняется для всех доступных адблоков

Пример: `check_detect.py --local_run --browsers chrome,firefox --pids autoru` проверит детект на auto.ru в хроме без записи результата в stat

На регулярной основе (2 часа) проверка запускается в [SandBox](https://a.yandex-team.ru/arc/trunk/arcadia/sandbox/projects/antiadblock/antiadblock_detect_checker/__init__.py)

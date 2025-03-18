# ADBlock Analytics Service

Состоит из 2х частей:
* модули
* код исполняющий модули

Работает следующим образом, загружает все модули и по очереди выполняет действия:
1) загрузка результатов предыдущих прогонов из YT
2) выполнения в модулях `get_check_result`
3) сравнение с предыщущим результатом - выполнение `filter_results`
4) если новый результат то выполняем ряд действий: письмо/тикет, и записываем новый результат в табличку


## Использование

### Использование из командной строки
При запуске утилиты можно использовать дополнительные команды, справку по которым видно при запуске `--help`:
```bash
ya make --checkout
./service/service --help
usage: analytics_service [-h] [--yt_token TOKEN] [--send_results]

Anti-AdBlock analytic tool. Gathering information about
adblocks/browsers/etc...

optional arguments:
  -h, --help        show this help message and exit
  --yt_token TOKEN  Using yt for storing checks results.
  --local_run       Run without sending results to YT
```

### Регулярный запуск
Сделан в sandbox запуском `ANTIADBLOCK_RUN_BINARY` таски с `binary_class`=`AntiadblockAnalyticsServiceBin`

## Релиз
```bash
# build:
ya package --checkout --target-platform default-linux-x86_64 --raw-package package.json
# release:
ya upload ./analytics_service.bin/service/service --type ANTIADBLOCK_ANALYTICS_SERVICE_BIN --ttl inf --owner ANTIADBLOCK --attr released=stable --description "antiadblock analytics_service binary"
```

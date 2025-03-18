# Тулза Антиадблока для проверики наличия у проксируемых пользователей блокировщиков (по логам браузера)

## Сборка и запуск
```bash
>> ya make --checkout
>> ./detect_verification -h
usage: detect_verification [-h] [--yt_token TOKEN] [--test]
                           [--update_uids_state]
                           [--barnavig_range xxxx-x-x xxxx-x-x]
                           [--uids_state_table TABLENAME] [--daily_logs]
                           [--dont_check_cryprox_uids]
                           [--cryprox_range xxxx-xx-xx* xxxx-xx-xx*]

Tool for checking if yabro antiadblock users have adblock (results pushed to
Solomon)

optional arguments:
  -h, --help            show this help message and exit
  --yt_token TOKEN      Expects oAuth token for YT as value
  --test                Will send results to Solomon testing if enabled
  --update_uids_state   If enabled, updates dynamic table with uniqid adblock
                        state
  --barnavig_range xxxx-x-x xxxx-x-x
                        Range of bar-navig-log files in yql request (Y-m-d
                        format)
  --uids_state_table TABLENAME
                        Use it if you dont want use default dynamic table with
                        uniqid adblock state
  --daily_logs          If endbled yql request will use logs/antiadb-nginx-
                        log/1d istead of logs/antiadb-nginx-log/stream/5min
  --dont_check_cryprox_uids
                        If you want just update uniqid state from bar-navig-
                        log
  --cryprox_range xxxx-xx-xx* xxxx-xx-xx*
                        Range of antiadb-nginx-log files in yql request. Y-m-d
                        format if --daily_logs else Y-m-dTH:M:S
```

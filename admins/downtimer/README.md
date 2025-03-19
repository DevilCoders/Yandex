# downtimer
Даунтаймер, который действительно даунтаймит

### Настройки
* Скрипт берет единый токен из переменной окружения `YDOWNTIMER_OAUTH`. Получить токен можно [здесь](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=2ca3c112656c4bfb9c2aef63bac9f75d)
  * LEGACY: Если переменной окружения нет - ищет по старому в `~/.runtime.yaml`:
```yaml
juggler_token: <токен к Juggler api>
yp_token: <токен к YP api (нужен, если в конфиге есть секция yp)>
qloud_token: <токен к Qloud api (нужен, если в конфиге есть секция qloud)>
nanny_token: <токен к Nanny api (нужен, если в конфиге есть секция nanny)>
abc_token: <токен к Abc api (нужен, если в конфиге для yp указываете slug сервиса, а не его id)>
```

Получить токены можно здесь (**Если вы используете единый токен - вам это не нужно**):
* [juggler_token](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0)
* [yp_token](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=f8446f826a6f4fd581bf0636849fdcd7)
* [qloud_token](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=072224ad7c864b158601e49b25497eac)
* [nanny_token](https://nanny.yandex-team.ru/ui/#/oauth/)
* [abc_token](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=23db397a10ae4fbcb1a7ab5896dc00f6)


* Скрипт оперирует с конфигом вида:
```yaml
qloud:
  - afisha # - проект в Qloud
conductor:
  - afisha # - проект в Conductor
yp: # - это YD (nanny контейнеры прорастать сюда не должны)
  - 109 # - id abc проекта, к которому привязан проект в YD
  - afisha # - slug abc проекта. Если указываете slug - у вас должен быть наш токен из YDOWNTIMER_OAUTH переменной, либо abc_token в ~/.runtime.yaml
  - "1234" # - это тоже slug, а не id
nanny:
    - include:
          /music # - nanny namespace
      exclude:
          - '^/music/back/dev/' # - regexp namespace'a, который надо исключить из выборки
          - '^/music/dev/'
          - '^/music/lyrics/'
          - '^/music/match/'
          - '^/music/yamrec/'
          - 'android'
          - 'balancer_pinger'
          - 'salt'
          - 'netlatency'
          - 'back_front'
```


### Запуск
* `ya make` чтобы получить бинарник или обновить существующий
* `./downtimer.py <путь до конфига: conf/test.yaml> dump vla 12h` - выгрузит список хостов для даунтайма в `dump.yaml`
* `./downtimer.py <путь до конфига: conf/test.yaml> set vla 12h` - проставит даунтайм на список хостов из файла `dump.yaml`

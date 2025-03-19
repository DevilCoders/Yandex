dps-proxy
=========	

[Fastcgi-daemon2](https://github.com/golubtsov/Fastcgi-Daemon) модуль для предоставления API к DPS mysql базе данных.

API [frozen]
------------

### **/ping**

Всегда возвращает `pong`.

### **/**
получение данных

##### GET параметры:
* `file` - имя получаемого файла (например: `/?file=/db/projects.xml`).
* `dir` - имя получаемой директории (например: `/?dir=/db` или `/?dir=/db/`).

Из двух параметров `file`, `dir` обязательно должен быть указан ровно один.

* [deprecated] `name` -  параметр имя файла. Если последний символ `/`, то распознается как `dir`, иначе распознается как `file`. `dir` и `file` имеют больший приоритет, чем `name`.
* `version` - версия файла / файлов в листинге диретории. Может быть stable, latest или конкретное число (номер версии).
* `removed` - форсировать чтение и показ в листинге удаленных файлов.
* `content` - добавлять ли в листинг директорий содержимое фалйов (в Base64).
* `recursive` - выдавать ли в листинге диреторий поддиретории.
* `format` - формат вывода листинга директорий: `xml` по умолчанию, `json`, `msgpack`.
* `directories` - включать ли в вывод списка директорий поддиректории, по умолчанию, `true`.
* `properties` - включать ли в вывод список свойств, по умолчанию, `true`.

### **/documentMetadata**
получение метаданных о файле 

##### GET параметры:
* `file` - имя получаемого файла (например: `/documentMetadata?file=/db/projects.xml`).

* `properties` - включать ли в вывод список свойств (по умолчанию `true`).
* `revisions` - включать ли в вывод список версий (без контента), по умолчанию `true`.

О пакете
--------

Пакет хранит конфигурационные файлы в `/etc/yandex/fastcgi-dps-proxy/configs/current`:

* dps-db.conf: хранит адреса до DPS MySQL баз.
* dps-proxy.env и dps-proxy.conf:   конфиги для fastcgi-daemon

Все функции работы с базой данных находятся в [`mysql_fs_layer`](https://github.yandex-team.ru/DPS/dps-proxy/blob/master/src/mysql_fs_layer.hpp). Этот файл предоставляет два самых главных метода - [`ReadFile`](https://github.yandex-team.ru/DPS/dps-proxy/blob/master/src/mysql_fs_layer.cpp#L154) (получает информацию только о файле/директории) и [`ReadDir`](https://github.yandex-team.ru/DPS/dps-proxy/blob/master/src/mysql_fs_layer.cpp#L209) (читает содержимое директории).

Роутинг осуществляется в [`proxy.cpp`](https://github.yandex-team.ru/DPS/dps-proxy/blob/master/src/proxy.cpp).

Все ручки находятся в каталоге [`handles`](https://github.yandex-team.ru/DPS/dps-proxy/tree/master/src/handles).

Установка
---------

```bash
# Устанавливаем пакет
$ sudo apt-get install fastcgi-dps-proxy
# Запускаем cthdbc
$ sudo service fastcgi-daemon2 start dps-proxy

# Настраиваем **разработческий** nginx
$ sudo wget https://github.yandex-team.ru/DPS/dps-proxy/raw/master/configs/fastcgi-dps-proxy-dev.conf -O /etc/nginx/sites-available
$ sudo ln -sf /etc/nginx/sites-available/fastcgi-dps-proxy-dev.conf /etc/nginx/sites-enabled/fastcgi-dps-proxy-dev.conf
$ sudo service nginx restart
```


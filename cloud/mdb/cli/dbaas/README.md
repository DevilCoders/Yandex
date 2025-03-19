## Configuration

Run `dbaas init` and follow instructions to set up configuration profiles for different environments.

## Usage

See https://jing.yandex-team.ru/files/alex-burmak/dbaas_py.mp4 for usage overview.

## Bastion

1. Открываем туннель с локальной машины до целевой на ssh порт - `ssh -N -L 7777:<target_fqdn>:22 bastion.cloud.yandex.net`
2. Открываем туннель с локальной машины до целевой на целевой порт (использует открытый тунель открытый выше) - `ssh -N -L <target_port>:<target_fqdn>:<target_port> root@localhost -p 7777`
3. В конфиге dbaas.py правим целевые хосты на `127.0.0.1`.

Нюансы:
1. Если нам нужен мастер базы, то либо открываем туннели на каждую машину, либо сначала находим мастер и открываем только 2 туннеля.
2. Некоторые параметры не пишутся в конфиг (например, fqdn metadb) - их нужно править прямо в коде (`config.py`).

## Установка в нестандартную директорию

Если вас по каким-то причинам не устраивает стандартная директория установки (`~/mdb-scripts`), 
то вы можете её переопределить при помощи переменной окружения `DBAAS_TOOL_ROOT_PATH` при установке или же при помощи параметра -p (--path). 

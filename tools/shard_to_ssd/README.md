# shard_to_ssd
Script for coping and deletion shard on SSD on base search with locking.

Это простой скрипт, написанный для SEPE-14135. Скрипт должен помочь при копировании шардов на ssd и последующем удалении с ssd. Постановка задачи следующая:

1. На хосте поднимаются инстансы, которым для работы необходимы шарда.
2. Каждый шард может быть нужен нескольким инстансам.
3. При активации/подготовке сервиса необходимо скопировать шард на ssd, если его там нет.
4. При остановке сервиса шард должен быть удален с ssd, если он не нужен ни одному из других сервисов.

Данный скрипт помогает имплементировать данную логику. Идея состоит в том, что при копировании шарда на ssd создается файл в котором ведется учет инстансов, которым нужен шард. При удалении шарда проверяется нет ли других желающих работать с шардом, если нет, то шард удаляется, если есть, то ничего не происходит. Доступ к файлу с "пользователями" шарда защищен при помощи flock(). Копирование и удаление шарда осеществляется внутри скрипта, если какая-то операция не удалась, то выбрасывается python исключение. Вполне возможно, что случаи ошибок при копировании или удалении надо обработать особо.

Подразумевается, что скрипт быдет вызываться в install и uninstall hook ISS.

## Краткая справка:
```
usage: shard_to_ssd.py [-h] [-c | -r] [-l DIR] [-s DIR] -i NAME SHARD

Manipulate shards on ssd

positional arguments:
  SHARD                 Path to shard

optional arguments:
  -h, --help            show this help message and exit
  -c, --copy            Copy shard on ssh
  -r, --remove          Remove shard from ssh if it is not needed for other
                        services
  -l DIR, --lockdir DIR
                        Directory for placing lock files
  -s DIR, --ssddir DIR  Mountpoint of ssd disk
  -i NAME, --id NAME    Unique id of service
```

Для работы с скриптом надо знать:

1. Директорию для файла блокировок (туда же будут записываться те, кому нужен шард). Опция -l
2. Директорию для копирования шарда. Опция -s
3. Путь до шарда. Передается без опции как параметр. Путь не должен содержать последний слеш "/"
4. Идентификатор инстанса. Может быть любым, но необходимо, чтоб у каждого инстанса был свой. Опция -i

Для регистрации посльзователей шарда будет использован файл <shardname>.lock в директории <lockdir>.

## Примеры работы

### Инстанс копирует шард на ssd
```
[mih@work shard_to_ssd]$ ./shard_to_ssd.py -c -l tests/ -s tests/ssd -i instance1 tests/shard2
INFO:shard_to_ssd:Lock on file /home/mih/projects/shard_to_ssd/tests/shard2.lock acquired
INFO:root:Starting copy shard shard2 to ssd
INFO:shard_to_ssd:Lock on file /home/mih/projects/shard_to_ssd/tests/shard2.lock released
```

Шард скопирован в директорию tests/ssd. При этом создан файл tests/shard2.lock.
```
[mih@work shard_to_ssd]$ cat tests/shard2.lock 
instance1[mih@work shard_to_ssd]$ 
```

Видим, что файл нужен для instance1.

### Другой инстанс копирует шард на ssd
```
[mih@work shard_to_ssd]$ ./shard_to_ssd.py -c -l tests/ -s tests/ssd -i instance2 tests/shard2
INFO:shard_to_ssd:Lock on file /home/mih/projects/shard_to_ssd/tests/shard2.lock acquired
INFO:root:Shard shard2 is already on ssd
INFO:shard_to_ssd:Lock on file /home/mih/projects/shard_to_ssd/tests/shard2.lock released

[mih@work shard_to_ssd]$ cat tests/shard2.lock 
instance1
instance2[mih@work shard_to_ssd]$ 
```

Копирования не происходит, но мы регистрируем второго пользователя.

### Первый инстанс удаляет шард с ssd
```
[mih@work shard_to_ssd]$ ./shard_to_ssd.py -r -l tests/ -s tests/ssd -i instance1 tests/shard2
INFO:shard_to_ssd:Lock on file /home/mih/projects/shard_to_ssd/tests/shard2.lock acquired
INFO:root:Some services need shard: shard2 Do nothing.
INFO:shard_to_ssd:Lock on file /home/mih/projects/shard_to_ssd/tests/shard2.lock released
[mih@work shard_to_ssd]$ cat tests/shard2.lock 
instance2[mih@work shard_to_ssd]$ 
```

Шард еще требуется для  instance2 и не удаляется.

### Второй инстанс удаляет шард с ssd.
```
[mih@work shard_to_ssd]$ ./shard_to_ssd.py -r -l tests/ -s tests/ssd -i instance2 tests/shard2
INFO:shard_to_ssd:Lock on file /home/mih/projects/shard_to_ssd/tests/shard2.lock acquired
INFO:root:I am last user. Removing shard shard2 from args.ssddir
INFO:shard_to_ssd:Lock on file /home/mih/projects/shard_to_ssd/tests/shard2.lock released
[mih@work shard_to_ssd]$ cat tests/shard2.lock 
cat: tests/shard2.lock: No such file or directory
```

Больше шард никому не нужен, удаляем. Файл с пользователями тоже будет удален.

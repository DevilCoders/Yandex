# ansible-juggler-configs

### juggler check with ansible
https://wiki.yandex-team.ru/sm/juggler/ansible/

**Импортированные из juggler-generator конфиги лежат в каталоге ./imported**

## ansible-juggler-generator.sh
Структура безжалостно скопипастена у ребят из https://github.yandex-team.ru/cs-admin/ansible-juggler-configs и немножко обработана под нас.

Рекомендуетя ansible > 2.0.0, но < 2.3.0 + ansible_juggler2:
```bash
sudo pip install --index-url https://pypi.yandex-team.ru/simple/  ansible-juggler2 -U

```

```bash
./ansible-juggler-generator.sh kinopoisk kp-backend # применение конкретного конфига
./ansible-juggler-generator.sh kinopoisk            # применение всех конфигов проекта
```

Для работы с `ansible-juggler-generator.sh` нужно создать каталог `./projects/${project}/` и положить конфиги туда.
Примерный шаблон конфига лежит в `./templates`
Для работы так же нужен файл с переменными `./projects/${project}/vars` с минимум двумя переменными:
```
project: "kinopoisk"
resps: ["robot-cult", "dmokhatkin", "sergeyv", "chrono", "paulus"]
```

В файле `./group_vars/all` лежат все common-переменные

## ansible-generator.sh
скрипт для работы с импортированными из juggler-generator конфигами. Рекомендую адаптировать под `ansible-juggler-generator.sh`

Создание дампа с конфига с использованием конфигов juggler-generator:
```bash
ansible-playbook ./imported/migration/dump.yml --extra-vars "svn_path=~/work/svn/corba-configs/juggler-generator/config project=music"
```

При этом создается директория для проекта `./imported/${project}`.
Сейчас работает только, когда самая простая иерархия в svn. Т.е. в директории juggler-generator/config/$PROJECT лежат все конфиги и директория с template'ами.

## Возможные ошибки и пути их решения
### ERROR! no action detected in task
```
ERROR! no action detected in task. This often indicates a misspelled module name, or incorrect module path. 
...

The offending line appears to be:

  tasks:
    - juggler_check:
      ^ here
``` 

Скорее всего у вас ansible версии >= 2.3.

**Решение:**
1. Удалите текущую версию
``` sudo pip uninstall ansible ```
2. И установите рекомендуемую
``` sudo pip install ansible==2.2.2.0 ```
-----

Old

Т.к. пока мы не подключились к sandbox таску, который накатывает проверки, то можно загружать их локально. В корне лежит скрипт - ansible-generator.sh
Запускается из корня на весь проект или конкретную проверку.

    ./ansible-generator.sh music
    ./ansible-generator.sh music music-test-back.yml


Версии пакетов, с которыми все работает.
ansible-juggler=0.23
ansible=1.9.4-yandex1 - ставится из yandex-trusty. На xenial нет уже этой версии.



чтобы завести ansible-juggler, нужно:

xenial

    wget http://dist.yandex.ru/yandex-trusty/stable/all/ansible_1.9.4-yandex1_all.deb
    sudo dpkg -i ansible_1.9.4-yandex1_all.deb
    sudo apt-get -y install ansible-juggler
    sudo pip install --index-url https://pypi.yandex-team.ru/simple/  ansible-juggler -U

trusty

    sudo apt-get -y install ansible ansible-juggler
    sudo pip install --index-url https://pypi.yandex-team.ru/simple/  ansible-juggler -U

Уже приведенные к новому формату проверки лежат в converted

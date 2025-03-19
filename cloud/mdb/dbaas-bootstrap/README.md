# DBaaS bootstrap script

### Инструкция:
1. Сконфигурить пиллары `mdb_controlplane_compute_<env>` и `top.sls` (по аналогии с готовыми).
2. Заказать сертификат для балансера API и добавить его в пиллар internal API.
3. Добавить **internal_salt** в базу `salt` на `saltdb01` в таблицу `salt.master_hosts`,
добавить в таблицу `salt.master_suffixes` для него суффикс, по которому миньоны будут вешаться на него,
добавить для него пининг к конкретному salt-мастеру в таблицу `salt.minions`.
4. Создать группу `mdb_controlplane_compute_<env>` и все корневые группы для пользовательских баз,
которые указаны в пилларе для воркеров.
5. Настроить синхронизацию файлов через `lsyncd` в конфиге `/etc/lsyncd/lsyncd.conf.lua` на **external_salt**.
6. Счекаутить приватные конфиги (`make private`)
7. Запустить bootstrap через tox, например `tox -e deploy_preprod`.

#### Как настроить ssh-proxy

В нужном venv-окружении правим файл `.tox/deploy_preprod/lib/python3.6/site-packages/dbaas_worker/providers/ssh.py`
Находим строчку, где происходит ssh-connect и меняем, чтобы получилось примерно следущее:

`proxy = paramiko.ProxyCommand('nc -X 5 -x localhost:7777 {} 22'.format(connect_address))
ssh.connect(connect_address, username='root', pkey=ssh_pkey, timeout=1, banner_timeout=1, sock=proxy)
`

Дальше запускаем проброс:

`
ssh -fND 7777 bastion.cloud.yandex.net
`

#### Какие конфиги надо править
Чтобы не трогать лишнего, стоит убрать это лишнее из конфигов. Для этого из конфига бутстрапа (например `./private/bootstrap-preprod.conf`) убираем все неинтересующие нас хосты. Так же надо не забыть убрать из схемы конфига (`dbaas_bootstrap/utils/config.py`) обязательные поля, которые мы убрали. Помимо этого, убираем ненужные группы из функции `dbaas_bootstrap.bootstrap.Bootstrap.deploy`.
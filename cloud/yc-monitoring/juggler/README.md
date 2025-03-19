# Prepare environment
```
# 1. Install mkvirtualenv (if not installed)
$ sudo pip install "virtualenv<20.0"
$ sudo pip install virtualenvwrapper
# Add 'source /usr/local/bin/virtualenvwrapper.sh' to '.bashrc OR .bash_profile OR .profile' and restart shell.

# 2. Create virtual environment
$ mkvirtualenv -p /usr/bin/python2.7 ansible-juggler
(ansible-juggler) $ pip install --index-url https://pypi.yandex-team.ru/simple/ -r requirements.txt
```
# Available tags
* yc-compute-api-team
* yc-compute-node-team
* network
* slb
* api_gateway
* iam
* billing
* infra
* vpc-api
* cic-api
* cic-agent
* bastion
* local-proxy
* xds-provider
* core
* overlay
* selfhost-ci
# Dry run
```
$ ansible-playbook yc_checks_config_%CLUSTER%.yml --tags %YOUR_TAG% --check
```
# Verbose dry run (shows what properties will be changed)
```
$ ansible-playbook yc_checks_config_%CLUSTER%.yml --tags %YOUR_TAG% --check -vvv
```
# Authorize
If you want change any check, you should authorize yourself. To get authorization token follow [link](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0).

Then set token as environment variable:
```
$ export JUGGLER_OAUTH_TOKEN=YouOAuthToken
```

# Run (applies changes)
```
$ ansible-playbook yc_checks_config_%CLUSTER%.yml --tags %YOUR_TAG%
```

# Deploy only one check
* Add temporarily custom tag (e.g., `tags: ["tag"]`) to the aggregate (don't commit)
```

- name: 'juggler_check: some-check'
  juggler_check: ''
  args: "{{ silent_check | hash_merge(item, default_timed_limits) }}"
  with_items:
    - service: some-check
  tags: ["tag"]
```
* Run `ansible-playbook` with `--tags tag`

# Troubleshooting

If you encounter
```
+[NSValue initialize] may have been in progress in another thread when fork() was called. We cannot safely call it
or ignore it in the fork() child process. Crashing instead. Set a breakpoint on objc_initializeAfterForkError to debug.
```
error, try
`export OBJC_DISABLE_INITIALIZE_FORK_SAFETY=YES`

Original recipe from [ansible github](https://github.com/ansible/ansible/issues/49207)


# Быстрый старт с применением docker-контейнера
## Рабочее окружение
Необходимое окружение реализовано в докер-контейнере
Для сборки, достаточно выполнить команду:

```(bash)
make build
```
## Авторизация
Внимание, для применения изменений, необходимо получить OAuth и определить соответствующую переменную
Токен можно получить через браузер по следующей [ссылке](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0).
```(bash)
export JUGGLER_OAUTH_TOKEN=YouOAuthToken
make cli
```
## Деплой
После, становятся доступны следующие make цели:
test - протестировать все на всех стендах (применить конфигурацию режиме dry-run)
test-<ENV> - протестировать на каком-то конкретном стенде, например test-preprod
apply - протестироватьи и применить изменения на всех стендах
apply-<ENV> - протестировать и применить изменения на каком-то конкретном стенде, например test-preprod

Для включения подробного вывода (покажет изменения), выполняй с аргументом v=yes

Для запусков задач с определенным тегом, используй tags=<TAG1>,<TAGn>

Примеры (выполняются из шела контейнера):
```(bash)
make test-preprod
make test-prod v=yes
make test-prod v=yes tags=cgw,cgw-dc
make apply
```
## Полезное
* Подробнее о доступных целях сборки можно самостоятельно посмотреть в Makefile
* В обновленной версии juggler-sdk==0.7.4 и ansible-juggler2==1.20, добавлена поддержка [pronounce](https://docs.yandex-team.ru/juggler/aggregates/basics), рекомендую использовать

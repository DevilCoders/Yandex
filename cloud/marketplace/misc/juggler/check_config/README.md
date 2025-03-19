# Prepare environment
```bash
$ mkvirtualenv -p /usr/bin/python2.7 ansible-juggler
$ pip install --index-url https://pypi.yandex-team.ru/simple/ ansible==2.2.3.0 ansible-juggler2==1.14 juggler-sdk==0.5.2
```
For MacOS add to `~/.bash_profile`
```bash
export OBJC_DISABLE_INITIALIZE_FORK_SAFETY=YES
```

# Dry run
```bash
$ ansible-playbook yc_checks_config.yml --check
```
# Verbose dry run (shows what properties will be changed)
```bash
$ ansible-playbook yc_checks_config.yml --check -vvv
```
# Authorize
If you want change any check, you should authorize yourself. To get authorization token follow [link](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0).

Add token as environment variable:
```bash
export JUGGLER_OAUTH_TOKEN=YouOAuthToken
```

# Run (applies changes)
```bash
$ ansible-playbook yc_checks_config.yml
```
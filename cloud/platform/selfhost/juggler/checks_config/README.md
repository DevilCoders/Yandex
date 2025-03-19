# Prepare environment
1) [Install virtualenvwrapper](https://virtualenvwrapper.readthedocs.io/en/latest/install.html)

2) Configure virtualenv
```
$ mkvirtualenv -p /usr/bin/python2.7 ansible-juggler
# Disable Old DT API in Juggler(https://clubs.at.yandex-team.ru/monitoring/2538)
$ pip install --index-url https://pypi.yandex-team.ru/simple/ ansible==2.2.3.0 ansible-juggler2==1.16 juggler-sdk==0.6
```

or
```
$ pip install --user --index-url https://pypi.yandex-team.ru/simple/ ansible==2.2.3.0 ansible-juggler2==1.16 juggler-sdk==0.6
```
to intall it into `~/.local`.

2a) Enver the virtual env

```
$ source virtualenvwrapper.sh # adds workon
$ workon ansible-juggler
```

3) (optional)
If playbook run hangs on macOS with message
```
objc[22402]: +[__NSPlaceholderDate initialize] may have been in progress in another thread when fork() was called.
objc[22402]: +[__NSPlaceholderDate initialize] may have been in progress in another thread when fork() was called. We cannot safely call it or ignore it in the fork() child process. Crashing instead. Set a breakpoint on objc_initializeAfterForkError to debug.
```
Add `export OBJC_DISABLE_INITIALIZE_FORK_SAFETY=YES` to your shell rc file.
See [ansible github issue](https://github.com/ansible/ansible/issues/32499) for details


# Available tags
* ai-gateway
* api-gateway
* api-router
* cloudlogs
* cpl-router
* dpl01-router
* jaeger
* logging
* mdb-gateway
* serverless
* serverless-database
* xds
# Dry run
```
$ ansible-playbook ai-gateway.yml --check
```
# Verbose dry run (shows what properties will be changed)
```
$ ansible-playbook ai-gateway.yml --check -vvv
```
# Authorize
If you want change any check, you should authorize yourself. To get authorization token follow [link](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0).

Then set token as environment variable:
```
$ export JUGGLER_OAUTH_TOKEN=YouOAuthToken
```

# Run (applies changes)
```
$ ansible-playbook ai-gateway.yml
```

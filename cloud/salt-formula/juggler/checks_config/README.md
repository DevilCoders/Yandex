# Prepare environment
```
$ mkvirtualenv -p /usr/bin/python2.7 ansible-juggler
# Disable Old DT API in Juggler(https://clubs.at.yandex-team.ru/monitoring/2538)
$ pip install --index-url https://pypi.yandex-team.ru/simple/ ansible==2.2.3.0 ansible-juggler2==1.16 juggler-sdk==0.6
```
# Available tags
* compute
* network
* slb
* api_gateway
* iam
* billing
* infra
# Dry run
```
$ ansible-playbook yc_checks_config.yml --tags %YOUR_TAG% --check
```
# Verbose dry run (shows what properties will be changed)
```
$ ansible-playbook yc_checks_config.yml --tags %YOUR_TAG% --check -vvv
```
# Authorize
If you want change any check, you should authorize yourself. To get authorization token follow [link](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0).

Then set token as environment variable:
```
$ export JUGGLER_OAUTH_TOKEN=YouOAuthToken
```

# Run (applies changes)
```
$ ansible-playbook yc_checks_config.yml --tags %YOUR_TAG%
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

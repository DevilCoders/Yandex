# mysql-configurator
The MySQL configurator, python fourth version

This is a python rewrite of the old perl mysql configurator suite.
Teamcity: https://teamcity.yandex-team.ru/project.html?projectId=MediaAdmins_MysqlConfigurator
**Now using Trendbox CI** (https://github.yandex-team.ru/search-interfaces/trendbox-ci/blob/master/docs/drafts/format%230.2/configuration-reference.md)
Conductor: https://c.yandex-team.ru/packages/mysql-configurator-4
Doc: https://wiki.yandex-team.ru/cult-admin/mysql-configurator-v-4/

**Config example**:
```
# all boolean options(exclude oldformat) and API host can be overrided in command line
level: 'INFO'
# use cache by default
cached: False
# use legacy method secdist or local file
legacy: True
# root path for git clone if grants config saved in git
local_path_root: /tmp/
# use this API host to determine config path
apihost: 'c.yandex-team.ru'
# root directory to save cache
cache_root: /tmp/
# secdist path, local file or relative path in git a repository
path: 'mysql/net/yandex/tv/tst/tv/dbs_converted'
# git url
url: 'git@github.yandex-team.ru:salt-media/content-secure.git'
# ignore http requests errors. If true any error(404 code is error too) raises exception
ignore_errors: False
```



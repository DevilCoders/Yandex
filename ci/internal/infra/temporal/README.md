## Подключение
Установка клиента - `brew install tctl`

* Тестинг - `tctl --ad dns:///ci-temporal-frontend-testing.in.yandex.net:7233 cluster health`
* Тестинг - `tctl --ad dns:///ci-temporal-frontend-prestable.in.yandex.net:7233 cluster health`
* Тестинг - `tctl --ad dns:///ci-temporal-frontend.in.yandex.net:7233 cluster health`

## Сборка

Сборка происходит одним бандлом с бинарником и конфигом через ya package.

Конфиги берутся из репозитория, бинарник temporal-server'а перекладывается из ресурса с релизом.

##Обновление

Релизы - https://github.com/temporalio/temporal/releases
Перед обновлением надо просмотреть changelog.
!!!Важно!!! Мажорные версии требует накатки обновления схемы на базу (см далее).

Скачивается бинарник temporal_VERSION_linux_amd64.tar.gz
и заливается при помощи ```bash ya upload  --ttl=inf temporal_*_linux_amd64.tar.gz```
и указывается вот в [package.json](https://a.yandex-team.ru/arc_vcs/ci/temporal/package.json?rev=r9018624#L29)

Так же версию надо обновить в [temporal/setup/setup.sh](https://a.yandex-team.ru/arc_vcs/ci/internal/infra/temporal/setup/setup.sh?rev=r9201662#L4).


###Накатка изменений на testing

```
cd ~/arcadia/ci/internal/infra/temporal/setup
export DB_PASSWORD=`ya vault get version --rsa-private-key=/Users/andreevdm/.ssh/id_rsa --rsa-login=andreevdm  sec-01fs2657xmxfh6xy7bqf5epmrn --json |  jq -r '.value."db.password"'`
export ELASTIC_PASSWORD=`ya vault get version --rsa-private-key=/Users/andreevdm/.ssh/id_rsa --rsa-login=andreevdm  sec-01fs2657xmxfh6xy7bqf5epmrn --json |  jq -r '.value."elastic.admin.password"'`
./setup.sh -e testing
```


###Накатка изменений на prestable

```
cd ~/arcadia/ci/internal/infra/temporal/setup
export DB_PASSWORD=`ya vault get version --rsa-private-key=/Users/andreevdm/.ssh/id_rsa --rsa-login=andreevdm  sec-01fty6r6gr33r6j3tdb9t3ymd7 --json |  jq -r '.value."db.password"'`
export ELASTIC_PASSWORD=`ya vault get version --rsa-private-key=/Users/andreevdm/.ssh/id_rsa --rsa-login=andreevdm  sec-01fty6r6gr33r6j3tdb9t3ymd7 --json |  jq -r '.value."elastic.admin.password"'`
./setup.sh -e prestable
```


###Накатка изменений на stable

```
cd ~/arcadia/ci/internal/infra/temporal/setup
export DB_PASSWORD=`ya vault get version --rsa-private-key=/Users/andreevdm/.ssh/id_rsa --rsa-login=andreevdm  sec-01fty6sry5xs49m23je41h56gn --json |  jq -r '.value."db.password"'`
export ELASTIC_PASSWORD=`ya vault get version --rsa-private-key=/Users/andreevdm/.ssh/id_rsa --rsa-login=andreevdm  sec-01fty6sry5xs49m23je41h56gn --json |  jq -r '.value."elastic.admin.password"'`
./setup.sh -e stable
```

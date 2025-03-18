# PGaaS-specific Django backend

> Disclaimer: although used by some services, everything you see here is EXTREMELY unstable and should be used with a great deal of caution.

This packages extends the default PostgreSQL adapter in Django to cope with the some quirks of [PostgreSQL as a Service](https://wiki.yandex-team.ru/MDB/).

Table of Contents
=================

* [Installation](#installation)
* [Running Tests](#running-tests)
* [Features](#features)
  * [Retry on Errors](#retry-on-errors)
    * [Retries and Transactions](#retries-and-transactions)
  * [Custom Collation](#custom-collation)
  * [Multiple Hosts](#multiple-hosts)
  * [Database Shell](#database-shell)


## Installation

1. `pip install -i https://pypi.yandex-team.ru/simple/ django_pgaas`
2. Add `django_pgaas` to `INSTALLED_APPS`
3. Set up database backend:

        DATABASES = {
            'default': {
                'ENGINE': 'django_pgaas.backend',
                ...
            }
            ...
        }

## Running Tests

```bash
$ docker-compose build
$ docker-compose run --rm testapp tox
```


## Features

### Retry on Errors

It is a normal state of affairs that some queries in PGaaS fail with errors such as
* `pgbouncer cannot connect to server`
* `ERROR:  database removed` (see [MDB-1661](https://st.yandex-team.ru/MDB-1661))

and [more][1]. To our best knowledge, such failures can be catched as either `OperationalError` or `DatabaseError` (excluding subclasses of the latter.)

`django_pgaas` retries failed connection attempts and SQL queries.


#### Retries and Transactions
> New in django-pgaas 0.1

`django_pgaas` DOES NOT retry queries executed within `atomic` block. This includes all code inside views when `ATOMIC_REQUESTS` is enabled.
In that case use `atomic_retry`.

```python
atomic_retry(using=None, savepoint=True)
```

It is a drop-in replacement for `django.db.transaction.atomic()`, except that it only works as a decorator.
Executes the decorated function in a transaction and retries is as a whole in case of failure.

```python
from django_pgaas import atomic_retry

@atomic_retry()
def do_update():
    instance = Spam.objects.select_for_update().filter(egg=True)
    instance.foo = 'bar'
    instance.save()

do_update()
```



#### Settings
* `PGAAS_RETRY_ATTEMPTS = 3`

  Number of attempted retries for making new connections and queries.
* `PGAAS_RETRY_SLOT = .125`

  Random exponencial backoff slot in seconds.
* `PGAAS_USE_ZDM = False`

  Enable [zero-downtime-migrations](https://github.com/Smosker/zero-downtime-migrations) by [smosker@](http://staff/smosker)

[1]: https://wiki.yandex-team.ru/dbaas/faq/#vnormalnoustanovlennomsoedineniivypolnjajuzaprosivotvetpoluchajuserverconncrashedinvalidserverparameteriliquerywaittimeout.chtodelat
[2]: https://simply.name/pg-lc-collate.html


### Custom Collation

PGaaS uses `LC_COLLATE=C` on production databases. From the practical point of view this means broken sorting and case-insensitive lookups of non-ASCII data.

`django_pgaas` enables case-insensitive lookups `iexact`, `icontains`, `istartswith`, and `iendswith` by injecting custom collation into SQL queries. For example, `icontains` becomes:
```sql
UPPER(column COLLATE "en_US") LIKE UPPER('%pattern%' COLLATE "en_US")
```

#### Settings
* ``PGAAS_COLLATION = 'COLLATE "en_US"'``

  Collation string. It seems that `en_US` is the only out-of-the-box collation supporting Unicode.

### Multiple Hosts

> New in django-pgaas 0.6

TL;DR: see a [complete example](examples/multihost_settings.py) of Django database settings for multiple hosts.

> **Context:** PostgreSQL >=10 supports putting [multiple hosts](https://paquier.xyz/postgresql-2/postgres-10-multi-host-connstr/) in a connection string. The client checks each host until it finds
> * a master server, if [`target_session_attrs`](https://www.postgresql.org/docs/devel/static/libpq-connect.html#LIBPQ-PARAMKEYWORDS) is set to `read-write`, or
> * any available server, if `target_session_attrs=any`.
>
>  It is currenly the [preferrable way](https://wiki.yandex-team.ru/mdb/api/#bazydannyx) to use multiple ‘bare’ hosts instead of a single proxy. Starting from [psycopg 2.7.3.2](http://initd.org/psycopg/articles/2017/10/24/psycopg-2732-released/) Django supports multiple comma-separated hosts in `'HOST'`.
>
> However, as the hosts are tried in order, one may want ensure that the first host in the list is always the nearest one for each instance of the application.

`django_pgaas` provides `HostManager` class that sorts a list of hosts from closest to farthest based on the current datacentre:
```python
# Current DC is `vla`
from django_pgaas import HostManager

hosts = [
    ('foo.yandex.net', 'sas'),
    ('bar.yandex.net', 'man'),
    ('baz.yandex.net', 'vla'),
]

manager = HostManager(hosts)

manager.host_string  # == 'baz.yandex.net,foo.yandex.net,bar.yandex.net'
```

The current DC is taken from the `QLOUD_DATACENTER` or `DEPLOY_NODE_DC` environment variable. If unset or invalid, hosts are returned in arbitrary order. It is possible to set the current DC explicitly:
```
HostManager(hosts, current_dc='man')
```

The class can be also initialised from the output of `yc mdb cluster ListHosts`:
```
hosts = [
    {
        "name": "foo.db.yandex.net",
        "options": {"geo": "sas", "type": "postgresql"}
    },
    {
        "name": "bar.db.yandex.net",
        "options": {"geo": "man", "type": "postgresql"}
    },
    {
        "name": "baz.db.yandex.net",
        "options": {"geo": "vla", "type": "postgresql"}
    },
]

manager = HostManager.create_from_yc(hosts)
```

Random ordering for hosts with the same location can be enabled via `randomize_same_dc` init parameter:

```python
HostManager(hosts, current_dc='man', randomize_same_dc=True)
```
or
```python
manager = HostManager.create_from_yc(hosts, randomize_same_dc=True)
```

### Database Shell

When multiple hosts are configured, `manage.py dbshell` sometimes fails make a writable connection. This is because the command ignores `target_session_attrs`, which in turn defaults to `any`.

`django_pgaas` provides `manage.py db` command. It is a drop-in replacement for `dbshell` except that it uses all parameters from `DATABASES` setting, including `target_session_attrs`. Thus, if your default database always points to master, so will `manage.py db`.

As a bonus, the command allows one to run queries from the command line:

```bash
$ ./manage.py db 'SELECT 25 + 17 as result;'

 result
--------
     42
(1 row)
```

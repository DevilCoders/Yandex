django_mds
==========

MDS (MeDia Storage) support for Django-based projects


Up version
==========

- `releaser changelog`

- Up version manually in `setup.py` and `ya.make`


Settings
========

There are few settings to set in `django.conf.settings` that define the behaviour of the django_mds.


### Storage settings

https://wiki.yandex-team.ru/mds/dev/protocol/

``MDS_HOST``
------------

default: ``None``

required: ``True``

Storage host for read access.


``MDS_WRITE_HOST``
------------------

default: ``None``

required: ``True``

Storage host for write access.


``MDS_NAMESPACE``
-----------------

default: ``None``

required: ``True``

Namespace of the service.


``MDS_EXPIRE``
--------------

default: ``None``

Adds the `expire` parameter in `params` dict, which is passed into POST-request to storage.


``MDS_USE_TVM2``
----------------

default: ``False``

If set to ``True`` the service will receive a TVM2 ticket and put it in the `X-Ya-Service-Ticket` header.
When it is set to ``True``, it requires the following settings also to be set:
`MDS_STORAGE_TVM2_CLIENT_ID`, `MDS_CLIENT_TVM2_CLIENT_ID`, `MDS_CLIENT_TVM2_SECRET`, `MDS_CLIENT_TVM2_BLACKBOX_CLIENT`.


``MDS_MAX_RETRIES``
-------------------

default: ``Retry(total=5, method_whitelist=('HEAD', 'GET', 'POST'), status_forcelist=(500, 502), backoff_factor=0.1)``

`urllib3.Retry` object, sets the retry policy when connecting to the storage.
Passed as a function argument `max_retries` during session initialization.


``MDS_TIMEOUT``
---------------

default: ``5``

Storage connection timeout.


### TVM2 Auth

https://wiki.yandex-team.ru/mds/auth/#tvm2

``MDS_STORAGE_TVM2_CLIENT_ID``
------------------------------

default: ``None``

TVM2 client ID of <ins>the storage</ins> that the service is accessing.
Passed as a single-object tuple in `destinations` function argument of TVM2 client.
Used only when `MDS_USE_TVM2` is set to `True`.


``MDS_CLIENT_TVM2_CLIENT_ID``
-----------------------------

default: ``None``

TVM2 client ID of <ins>the service</ins> that accessing the storage.
Passed as `client_id` function argument to TVM2 client.
Used only when `MDS_USE_TVM2` is set to `True`.


``MDS_CLIENT_TVM2_SECRET``
--------------------------

default: ``None``

TVM2 secret of the service that accessing the storage.
Passed as `secret` function argument to TVM2 client.
Used only when `MDS_USE_TVM2` is set to `True`.


``MDS_CLIENT_TVM2_BLACKBOX_CLIENT``
-----------------------------------

default: ``None``

BlackBox TVM2 client ID.
Passed as `blackbox_client` function argument to TVM2 client.
Used only when `MDS_USE_TVM2` is set to `True`.


### Basic Auth (deprecated)

https://wiki.yandex-team.ru/mds/auth/#basicauthdeprecated

``MDS_ACCESS_TOKEN``
--------------------

default: ``None``

required: ``True``

Base authorization access token for read and write access (if not redefined in ``MDS_READ_TOKEN`` or ``MDS_WRITE_TOKEN``).


``MDS_WRITE_TOKEN``
-------------------

default: ``MDS_ACCESS_TOKEN``

required: ``True``

Base authorization access token for write access.
If not specified explicitly, taken from `MDS_ACCESS_TOKEN`.


``MDS_READ_TOKEN``
------------------

default: ``MDS_ACCESS_TOKEN``

Base authorization access token for read access.
If not specified explicitly, taken from `MDS_ACCESS_TOKEN`.

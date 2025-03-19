# -*- coding: utf-8 -*-
"""
This module helps to use Yandex Vault with salt.

:depends: yandex-passport-vault-client (https://vault-api.passport.yandex.net/docs/#yav)
It can be installed with
.. code-block:: bash

    $ pip install yandex-passport-vault-client -i https://pypi.yandex-team.ru/simple

This is often useful if you wish to store your pillars in source control or
share your pillar data with others but do not store secrets in them.

:configuration: The following configuration should be provided
    define (pillar or config files) Check that private keyfile is owned by root and has correct chown (400)`:

    .. code-block:: python

        # cat /etc/salt/master.d/yav.conf
        yav.config:
            rsa-private-key: /root/.ssh/id_rsa
            rsa-login: your-robot-login

For masterless configurations (e.g.: salt bootstrapping) you can use ssh-agent:
    .. code-block:: python
        # cat /etc/salt/master.d/yav.conf
        yav.config:
            agent: True

Pillar files can include yav secret uids and they will be replaced by secret values while compilation:

.. code-block:: jinja

    pillarexample:
        user: root
        some-token-list: {{salt.yav.get('ver-01cr01gy78qv1a33rtp538ryyc')|json}}
        some-token: {{salt.yav.get('ver-01cr01gy78qv1a33rtp538ryyc[TOKEN]')|json}}
        server-certificate: {{salt.yav.get('sec-01cqywscqcd9jed163wv1nbzx8[server.crt]')|json}}
        server-certificate-key: {{salt.yav.get('sec-01cqywscqcd9jed163wv1nbzx8[server.key]')|json}}

If specific key not set then dict will be returned.
In pillars rended with jinja be sure to include `|json` so line breaks are encoded:

.. code-block:: jinja

    cert: "{{salt.yav.get('S2uogToXkgENz9...085KYt')|json}}"

In states rendered with jinja it is also good pratice to include `|json`:

.. code-block:: jinja

    {{sls}} private key:
        file.managed:
            - name: /etc/ssl/private/cert.key
            - mode: 700
            - contents: {{pillar['pillarexample']['server-certificate-key']|json}}

"""
import base64
import re
import fnmatch
from collections import OrderedDict
from typing import Any

from library.python.vault_client.auth import RSAPrivateKeyAuth, RSASSHAgentAuth
from library.python.vault_client.errors import ClientError
from library.python.vault_client.instances import Production

__salt__: dict[str, Any] = {}
__opts__: dict[str, Any] = {}
__context__: dict[str, Any] = {}

__virtualname__ = "yav"

YAV_RE = re.compile(
    r"^(?P<uuid>(?:sec|ver)-[0-9a-z]{26,})(?:\[(?P<keys>.+?)\])?$", re.I
)


def __virtual__():
    return __virtualname__


def _get_config(**kwargs) -> dict[str, Any]:
    """
    Return configuration
    """
    config = {"rsa-private-key": None, "rsa-login": None, "agent": None}

    config_key = f"{__virtualname__}.config"
    try:
        config.update(__salt__["config.get"](config_key, {}))
    except (NameError, KeyError):
        # likly using salt-run so fallback to __opts__
        config.update(__opts__.get(config_key, {}))

    for k in set(config.keys()) & set(kwargs.keys()):
        config[k] = kwargs[k]

    if "yav_cache" not in __context__:
        __context__["yav_cache"] = dict()

    return config


def _get_yav_client(**kwargs):
    if "yav_client" in __context__:
        return __context__["yav_client"]

    # get config
    config = _get_config(**kwargs)

    vault_client = None
    # create Vault client
    if config.get("oauth-token"):
        vault_client = Production(authorization=f"Oauth {config['oauth-token']}")
    elif config.get("agent"):
        vault_client = Production(rsa_auth=RSASSHAgentAuth())
    elif config.get("rsa-login") and config.get("rsa-private-key"):
        try:
            # read rsa private key file
            with open(config["rsa-private-key"], "r") as keyf:
                rsa_pkey = keyf.read().rstrip("\n")
        except Exception as e:
            raise Exception(f"YaV RSA key configuration error({e}).")

        try:
            ssh_key = RSAPrivateKeyAuth(rsa_pkey)
        except Exception as e:
            raise Exception(f"YaV RSA key configuration error({e}).")
        else:
            vault_client = Production(
                rsa_auth=ssh_key,
                rsa_login=config.get("rsa-login"),
            )

    if not vault_client:
        raise Exception(
            "YaV client configuration error. Check if oauth token or login and private key path provided."
        )

    __context__["yav_client"] = vault_client
    return vault_client


def get(data, **kwargs):
    matches = YAV_RE.match(data.strip())
    if not matches:
        # why ???
        return data

    secret_uuid = matches.group("uuid")
    keys = (
        [s.strip() for s in matches.group("keys").split(",")]
        if matches.group("keys")
        else None
    )

    # по умолчанию True - сохраняем старое поведение как в python2.7
    decode_values = kwargs.pop("decode", True)

    vault_client = _get_yav_client(**kwargs)

    value = None
    if secret_uuid in __context__["yav_cache"]:
        value = __context__["yav_cache"][secret_uuid]

    if value is None:
        try:
            value = vault_client.get_version(secret_uuid, packed_value=False)["value"]
            __context__["yav_cache"][secret_uuid] = value
        except ClientError as e:
            raise Exception(
                f"{secret_uuid}: {e.kwargs.get('message')} (req: {e.kwargs.get('request_id')})"
            )

    packed_value = OrderedDict()
    for v in value:
        processed_value = v["value"]
        encoding = v.get("encoding")
        if encoding and encoding == "base64":
            processed_value = base64.b64decode(processed_value)

        if decode_values and isinstance(processed_value, bytes):
            processed_value = processed_value.decode()
        packed_value[v["key"]] = processed_value

    if keys:
        result = set()
        val_keys = list(packed_value.keys())
        for k in keys:
            result.update(fnmatch.filter(val_keys, k))

        packed_value = {x: packed_value[x] for x in result}
        if len(keys) == 1:
            return packed_value.get(keys[0])
    return packed_value

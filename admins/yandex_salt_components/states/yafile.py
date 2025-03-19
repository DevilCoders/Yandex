# -*- coding: utf-8 -*-
"""
Wrapper state under standart 'file' state than supports automatic source lookup
Example this state:
    ```sls
    file.managed:
        - source:
          {% if yaenv == "testing" or group == "some-testing-group" }
          - salt://my/fyle
          {% else %} {# production #}
          - salt://my/file
          {% endif %}
    ```
with yafile.managed is simple:
    ```sls
    yafile.managed:
      - source: salt://my/file
    ```
but require files with suffixes into salt roots
```
    /salt/roots/my/file
    /salt/roots/my/file-testing
```

yafile automatically expand every source to list of posible sources like below:
    ```sls
    - salt://my/file-fqdn.target.host
    - salt://my/file-group-target-host@datacenter
    - salt://my/file-group-target-host@root_datacenter
    - salt://my/file-group-target-host
    - salt://my/file-yandex-environment  # from /etc/yandex/environment
    - salt://my/file                     # original source
    ```

more about file.managed
https://docs.saltstack.com/en/latest/ref/states/all/salt.states.file.html#salt.states.file.managed
"""

import logging
from typing import Any

log = logging.getLogger(__name__)

__grains__: dict[str, Any] = {}
__opts__: dict[str, Any] = {}
__salt__: dict[str, Any] = {}


def _error(ret, err_msg):
    ret["result"] = False
    ret["comment"] = err_msg
    return ret


def _gen_sources(skip_dc, sources):
    new_sources = []
    group = __grains__["conductor"]["group"]

    suffixes = [__grains__["fqdn"]]
    if not skip_dc:
        suffixes.extend(
            [
                f"{group}@{__grains__['conductor']['datacenter']}",
                f"{group}@{__grains__['conductor']['root_datacenter']}",
            ]
        )
    suffixes.extend([group, __grains__["yandex-environment"]])
    for s in sources:
        new_sources.extend([f"{s}-{suffix}" for suffix in suffixes])
        new_sources.append(s)

    return new_sources


def _shim(func_name: str, name: str, kwargs: dict[str, Any]):
    import salt.states.file

    salt.states.file.__salt__ = __salt__  # type: ignore # noqa
    salt.states.file.__opts__ = __opts__  # type: ignore # noqa
    salt.states.file.__grains__ = __grains__  # type: ignore # noqa
    salt.states.file.__pillar__ = __pillar__  # type: ignore # noqa
    salt.states.file.__env__ = __env__  # type: ignore # noqa
    salt.states.file.__instance_id__ = __instance_id__  # type: ignore # noqa

    ret = {"changes": {}, "comment": "", "name": name, "result": True}
    possible_sources = kwargs.get("source", [])
    if isinstance(possible_sources, str):
        possible_sources = [possible_sources]
    log.info("Source is %s", possible_sources)

    skip_dc = kwargs.pop("skip_dc", False)
    log.info("yafile skip_dc = %s", skip_dc)

    if possible_sources:
        try:
            possible_sources = _gen_sources(skip_dc, possible_sources)
        except KeyError as e:
            where = ""
            if str(e).strip("\"'") in [
                "conductor",
                "datacenter",
                "root_datacenter",
                "group",
            ]:
                where = "conductor "
            return _error(ret, f"Please actualize {where}{e} grains for this host.")

        log.info("Running file.%s with sources: %s", func_name, possible_sources)
        kwargs["source"] = possible_sources

    return getattr(salt.states.file, func_name)(name, **kwargs)


def managed(name, **kwargs):
    return _shim("managed", name, kwargs)


def recurse(name, **kwargs):
    return _shim("recurse", name, kwargs)

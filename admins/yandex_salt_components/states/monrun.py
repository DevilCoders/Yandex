#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Mange monrun configuration
==========================

Control the yandex monitoring helper.
Creates/deletes monrunt config and regenerates Snaked tasks.
Only 'command' and 'name' are mandatory.
By default 'file' is '/etc/monrun/conf.d/' + name + '.conf'
"""

# Import python libs
import os
import difflib
import traceback
import logging
import configparser
from collections import OrderedDict
from typing import Any

__opts__: dict[str, Any] = {}

log = logging.getLogger(__name__)

__virtualname__ = "monrun"


def __virtual__():
    return __virtualname__


def _error(ret, err_msg):
    ret["result"] = False
    ret["comment"] = err_msg
    return ret


def _dump_config(cfg):
    sections = cfg.sections()
    sections.sort()
    result = []
    for sec in sections:
        if result:
            result.append("\n")  # 2 '\n' between sections
        result.append("[{0}]\n".format(sec))
        result.extend("{0} = {1}\n".format(*i) for i in cfg.items(sec))

    return result


def _add_section(cfg, key, data):
    cfg.add_section(key)
    for k in sorted(data):
        cfg.set(key, k, str(data[k]).strip())


def present(
    name,
    command=None,
    file=None,
    checks={},
    replace=True,
    options=None,
    regenerate_tasks=True,
    **kwargs
):
    """
    .. code-block:: yaml

      mysql_slave:
          monrun.present:
            - command: /usr/bin/check_mysql_slave
            - file: /etc/monrun/conf.d/mysql_slave.conf
            - execution_interval: 60
            - execution_timeout: 30
            - any_name: any_value

      # With many check
      mysql:
          monrun.present:
            - type: mysql
            - any_name: any_value
            - checks:
                replica_lags:
                  command: /usr/bin/check_mysql_slave
                  execution_interval: 60
                  execution_timeout: 30
                connections:
                  command: /usr/bin/check_mysql_con
                  execution_interval: 60
                  execution_timeout: 30
                  any_name: any_value

      # Many check, with single command but options
      mysql:
          monrun.present:
            - type: mysql
            - command: /usr/bin/check_mysql
            - any_name: any_value
            - execution_interval: 90
            - checks:
                replica_lags:
                  options: lags
                  execution_timeout: 50
                connections:
                  options: connections
                  any_name: any_value
    """

    ret = {"name": name, "changes": {}, "result": True, "comment": ""}

    if options:
        return _error(ret, "Parameter 'options' allowed only inside checks")

    monitoring_conf = {
        "execution_interval": 60,
        "execution_timeout": 30,
    }

    for kwarg in kwargs.keys():
        if kwarg in (
            "__id__",
            "fun",
            "state",
            "__env__",
            "__sls__",
            "order",
            "watch",
            "watch_in",
            "require",
            "require_in",
            "prereq",
            "prereq_in",
        ):
            pass
        else:
            monitoring_conf[kwarg] = kwargs[kwarg]

    if file is None:
        file = "/etc/monrun/conf.d/{0}.conf".format(name)
    else:
        if not file.startswith("/etc"):
            file = os.path.join("/etc/monrun/conf.d", file)

    if os.path.islink(file):
        if not replace:
            return _error(ret, "{0} is link and replace not allowed!".format(file))
        os.unlink(file)
        ret["changes"]["old"] = "Link {0}".format(file)
        ret["changes"]["new"] = "Monrun config {0}".format(file)

    old_config = None
    if os.path.exists(file):
        old_config = configparser.ConfigParser(None, OrderedDict, interpolation=None)
        if not replace:
            return _error(ret, "File {0} exists and replace not allowed!".format(file))

        if os.path.isfile(file):
            old_config.read(file)
        elif os.path.isdir(file):
            return _error(ret, "Specified target {0} is a directory".format(name))

    if checks is None or not isinstance(checks, dict):
        return _error(ret, "Checks must be formed as a dict")

    new_config = configparser.ConfigParser(None, OrderedDict, interpolation=None)
    for key in sorted(checks):
        values = dict(checks[key])
        for global_key in monitoring_conf:
            if global_key not in values:
                values[global_key] = monitoring_conf[global_key]
        if "command" not in values:
            if not command:
                err = "Option 'command' must be specified for check or globally"
                return _error(ret, err)
            values["command"] = command.strip()
        try:
            values["command"] += " " + str(values.pop("options")).strip()
        except KeyError:
            pass
        _add_section(new_config, key, values)

    if not checks:  # build one sections with name = `name'
        values = dict(monitoring_conf)
        values["command"] = command.strip()
        _add_section(new_config, name, values)

    # After adding all values, dump config as list of lines ends with \n
    new_text = _dump_config(new_config)

    if old_config is None:  # Config not exists
        ret["changes"]["diff"] = "New file"
    else:
        old_text = _dump_config(old_config)

        diff = "".join(difflib.unified_diff(old_text, new_text))
        if not diff:
            ret["result"] = True
            ret["comment"] = "Monrun config {0} is in the correct state".format(name)
            return ret

        ret["changes"]["diff"] = diff

    ret["result"] = None

    if __opts__["test"]:
        ret["comment"] = "Monrun config {0} is set to be changed".format(name)
        return ret

    try:
        with open(file, "w") as cf_fh:
            new_config.write(cf_fh)
    except Exception as exc:
        ret["changes"] = {}
        log.debug(traceback.format_exc())
        return _error(ret, "Unable to manage monrun config: {0}".format(exc))

    ret["comment"] = "Monrun config {0} updated.".format(file)

    # Сгенерируем новые snaked-джобы
    # FIXME: стейт сфейлится только в первый раз!
    # XXX UPD:  Что здесь нужно зафиксить?
    if regenerate_tasks:
        ret["result"] = _regenerate_tasks()
        if not ret["result"]:
            ret["comment"] += "\nFailed to regenerate monrun tasks!"

    return ret


def absent(name, file=None, regenerate_tasks=True, **kwargs):
    """
    .. code-block:: yaml
      "Remove obsolete monrun file":
        monrun.absent:
            - file: /etc/monrun/conf.d/mysql_slave.conf
    """

    import salt.states.file

    salt.states.file.__salt__ = __salt__  # type: ignore # noqa
    salt.states.file.__opts__ = __opts__  # type: ignore # noqa
    salt.states.file.__grains__ = __grains__  # type: ignore # noqa
    salt.states.file.__pillar__ = __pillar__  # type: ignore # noqa
    salt.states.file.__env__ = __env__  # type: ignore # noqa

    if file is None:
        file = "/etc/monrun/conf.d/{0}.conf".format(name)
    else:
        if not file.startswith("/etc"):
            file = os.path.join("/etc/monrun/conf.d", file)

    ret = salt.states.file.absent(file)
    if ret["changes"] and not globals()["__opts__"]["test"]:
        if regenerate_tasks:
            ret["result"] = _regenerate_tasks()
            if not ret["result"]:
                ret["comment"] = ret["comment"] + "\nFailed to regenerate monrun tasks!"
    return ret


def _regenerate_tasks():
    regenerate_bin = "/usr/sbin/regenerate-monrun-tasks >/dev/null 2>&1"

    result = os.system(regenerate_bin)
    if result == 0:
        log.info("Regenerating snaked jobs succeeded.")
        return True
    else:
        log.info("Regenerating snaked jobs failed.")
        return False

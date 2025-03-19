"""
Init systems control module
"""

import enum
import logging
from abc import abstractmethod
from typing import Dict, Type

from mdb_mongo_tools.util import exec_command


class CmdCtl(enum.Enum):
    """
    Command enums
    """

    START = 'start'
    STOP = 'stop'


class BaseServiceCtl:
    """
    Service control base class
    """

    def command(self, service: str, cmd: CmdCtl) -> None:
        """
        Run command
        """
        full_cmd = self.template.format(template=self.template, cmd=cmd.value, service=service)
        retcode, stdout, stderr = exec_command(full_cmd, timeout=10)
        if retcode:
            logging.critical('Unexpected execution: "%s" returned %s\nstdout:%s\nstderr:%s', full_cmd, retcode, stdout,
                             stderr)

    @property
    @abstractmethod
    def template(self) -> str:
        """
        Command template
        """


class SupervisorCtl(BaseServiceCtl):
    """
    Supervisord control
    """

    @property
    def template(self) -> str:
        return '/usr/bin/supervisorctl {cmd} {service}'


class SysvinitCtl(BaseServiceCtl):
    """
    sysvinit-utils control, compatible with systemd
    """

    template = '/usr/sbin/service {service} {cmd}'


def get_service_ctl_instance(rtype: str) -> BaseServiceCtl:
    """
    Return class instance for required type of service ctl
    """

    impl: Dict[str, Type[BaseServiceCtl]] = {
        'sysvinit': SysvinitCtl,
        'supervisor': SupervisorCtl,
    }
    req_class = impl.get(rtype, None)

    if req_class:
        return req_class()

    raise NotImplementedError('Unknown instance type: {rtype}'.format(rtype=rtype))

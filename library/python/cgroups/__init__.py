# -*- coding: utf-8 -*-
import os
import sys
import pwd
import getpass

from functools import reduce

import six

from six.moves import filter, map, range

CGROUPS_ROOT = "/sys/fs/cgroup"
CGROUP_SUBSYSTEMS = frozenset(["cpuacct", "cpu", "cpuset", "blkio", "memory", "freezer", "devices"])


class CGroup(object):
    """
    Representation of linux control groups (cgroups)

    Usage examples:

    .. code-block:: python

        ## cgroups of the current process
        cgroup = CGroup()

        ## cgroups of the specific process
        cgroup = CGroup(pid=<process id>)

        ## cgroups with path relative to cgroups of the current process
        cgroup = CGroup("custom/path")

        ## cgroups with path relative to cgroups of the specific process
        cgroup = CGroup("custom/path", pid=<process id>)

        ## cgroups with specific absolute path
        cgroup = CGroup("/custom/path")

        ## OS does not support cgroups
        assert CGroup() is None

        ## specification cgroup to subsystems
        cgroup.cpu
        cgroup.cpuacct
        cgroup.devices
        cgroup.freezer
        assert cgroup.freezer.name == "freezer"
        cgroup.memory
        cgroup["memory"]
        ...
        for subsystem in cgroup:  # iterate over all subsystems of the cgroup
            ...

        ## changing path of the cgroup
        cgroup = CGroup("/custom/path")
        assert cgroup.name == "/custom/path"
        cgroup2 = cgroup >> "subpath"
        assert cgroup2.name == "/custom/path/subpath"
        assert (cgroup << 1).name == "/custom"
        assert (cgroup2 << 2).name == "/custom"
        assert (cgroups.memory >> "subpath").cgroup.name == "/custom/path/subpath"
        assert (cgroups2.cpu << 2).cgroup.name == "/custom"

        ## create cgroup for all subsystems, does nothing if already exists
        cgroup = CGroup().create()
        assert cgroup.exists

        ## create cgroup for specific subsystem, does nothing if already exists
        cgroup = CGroup().freezer.create()
        assert cgroup.exists

        ## delete cgroup for all subsystems, does nothing if not exists
        cgroup = CGroup().delete()
        assert not cgroup.exists

        ## delete cgroup for specific subsystem, does nothing if not exists
        cgroup = CGroup().freezer.delete()
        assert not cgroup.exists

        ## check the occurrence of the process in the cgroup
        cgroup = CGroup()
        assert os.getpid() in cgroup  # process there is at least in one of subsystem

        ## add process to all subsystems of the cgroup
        cgroup = CGroup("custom/path")
        cgroup += <pid>
        assert <pid> in cgroup
        assert all(<pid> in subsystem for subsystem in cgroup)

        ## add process to specific subsystem of the cgroup
        freezer = CGroup("custom/path").freezer
        freezer += <pid>
        assert <pid> in freezer
        # or
        freezer.tasks = <pid>
        assert <pid> in freezer.tasks

        ## changing cgroup's limits
        cgroup = CGroup()
        cgroup.memory["limit_in_bytes"] = "11G"
        assert cgroup.memory["limit_in_bytes"] == "11G"
    """
    ROOT = "/sys/fs/cgroup"
    EXCLUDE_SUBSYSTEMS = []

    @property
    class Subsystem(object):
        __name = None

        # *_ added just for dummy PyCharm
        def __init__(self, cgroup, *_):
            self.__cgroup = cgroup

        def __call__(self, name, pid=None, owner=None):
            assert name
            self.__name = name
            self.__owner = owner
            if pid is not None:
                subsys_root = os.path.join(CGroup.ROOT, self.__name)
                for rootpath, dirnames, _ in os.walk(subsys_root):
                    for dirname in dirnames:
                        dirpath = os.path.join(rootpath, dirname)
                        tasks_path = os.path.join(dirpath, "tasks")
                        if not os.path.exists(tasks_path):
                            continue
                        with open(os.path.join(dirpath, "tasks")) as f:
                            if pid not in map(int, f.readlines()):
                                continue
                        return type(self.__cgroup)(
                            os.path.join(dirpath, self.__cgroup.name.lstrip("/"))[len(subsys_root):],
                            owner=owner
                        )[self.__name]
            return self

        def __repr__(self):
            return "<{}.{}({}: {})>".format(
                type(self.cgroup).__name__, type(self).__name__, self.__name, self.__cgroup.name
            )

        def available(self):
            if not sys.platform.startswith("linux"):
                return False
            if not os.path.exists(CGROUPS_ROOT):
                return False
            for subsystem in CGROUP_SUBSYSTEMS:
                checked_path = "{}/{}/{}".format(CGROUPS_ROOT, subsystem, getpass.getuser())
                if not os.path.exists(checked_path):
                    checked_path = "{}/{}".format(CGROUPS_ROOT, subsystem)
                if not os.access(checked_path, os.R_OK | os.W_OK):
                    return False
            return True

        @property
        def name(self):
            return self.__name

        @property
        def cgroup(self):
            return self.__cgroup

        @property
        def path(self):
            return os.path.join(CGroup.ROOT, self.__name, self.__cgroup.name.lstrip("/"))

        @property
        def exists(self):
            return os.path.exists(self.path)

        @property
        def tasks(self):
            with open(os.path.join(self.path, "tasks")) as f:
                return list(map(int, f.readlines()))

        @tasks.setter
        def tasks(self, pid):
            self.create()
            with open(os.path.join(self.path, "tasks"), "wb") as f:
                f.write(six.ensure_binary("{}\n".format(pid)))

        def create(self):
            if not self.exists:
                os.makedirs(self.path, mode=0o755)
                if self.__owner:
                    os.chown(self.path, pwd.getpwnam(self.__owner).pw_uid, -1)
            return self

        def delete(self):
            if self.exists:
                for x in self:
                    x.delete()
                os.rmdir(self.path)
            return self

        def set_current(self):
            """ Place current process to the subsystem of cgroup """
            if self.exists:
                self.tasks = os.getpid()

        def __resource_path(self, resource):
            return os.path.join(self.path, ".".join((self.__name, resource)))

        def __getitem__(self, resource):
            with open(self.__resource_path(resource)) as f:
                return list(map(str.strip, f.readlines()))

        def __setitem__(self, resource, value):
            self.create()
            with open(self.__resource_path(resource), "wb") as f:
                f.write(six.ensure_binary("{}\n".format(value)))

        def __iter__(self):
            path = self.path
            for _ in os.listdir(path):
                if os.path.isdir(os.path.join(path, _)):
                    yield type(self.__cgroup)(os.path.join(self.__cgroup.name, _), owner=self.__owner)[self.__name]

        def __rshift__(self, name):
            return (self.__cgroup >> name)[self.__name]

        def __lshift__(self, level):
            return (self.__cgroup << level)[self.__name]

        def __contains__(self, pid):
            return (
                self
                if pid in self.tasks else
                next(filter(None, map(lambda _: _.__contains__(pid), self)), None)
            )

        def __iadd__(self, pid):
            self.tasks = pid
            return self

    def __new__(cls, *args, **kws):
        if os.path.isdir(cls.ROOT) and os.listdir(cls.ROOT):
            return super(CGroup, cls).__new__(cls)
        cls._not_avaialable()

    @classmethod
    def _not_avaialable(cls):
        raise Exception("cgroups not available")

    def _validate_subsystem(self, subsystem):
        if subsystem not in self.__subsystems:
            raise Exception("subsystem {} is not supported".format(subsystem))

    def __init__(self, name=None, pid=None, owner=None, subsystems=None):
        self.__name = (name or "").rstrip("/")
        self.__pid = pid if pid is not None or self.__name.startswith("/") else os.getpid()
        self.__owner = owner
        self.__subsystems = CGROUP_SUBSYSTEMS if subsystems is None else subsystems

    def __repr__(self):
        return "<{}({})>".format(type(self).__name__, self.__name)

    def __rshift__(self, name):
        return type(self)(os.path.join(self.__name, name) if self.__name else name)

    def __lshift__(self, level):
        return type(self)(reduce(lambda p, _: os.path.dirname(p), range(level), self.__name), owner=self.__owner)

    def __getitem__(self, subsystem):
        self._validate_subsystem(subsystem)
        return self.Subsystem(subsystem, self.__pid, self.__owner)

    def __iter__(self):
        for subsys in self.__subsystems:
            if subsys in self.EXCLUDE_SUBSYSTEMS:
                continue
            subsys_path = os.path.join(self.ROOT, subsys)
            if os.path.isdir(subsys_path) and os.path.exists(os.path.join(subsys_path, "tasks")):
                yield self.Subsystem(subsys, self.__pid, self.__owner)

    def __contains__(self, pid):
        return any(pid in subsys for subsys in self)

    def __iadd__(self, pid):
        for subsys in self:
            subsys.__iadd__(pid)
        return self

    @property
    def name(self):
        return self.__name

    @property
    def exists(self):
        return any(subsys.exists for subsys in self)

    def create(self):
        for subsys in self:
            subsys.create()
        return self

    def delete(self):
        for subsys in self:
            subsys.delete()
        return self

    def set_current(self):
        """ Place current process to all subsystems of the cgroup """
        for subsys in self:
            subsys.set_current()

    @property
    def cpu(self):
        self._validate_subsystem("cpu")
        return self.Subsystem("cpu", self.__pid, self.__owner)

    @property
    def cpuacct(self):
        self._validate_subsystem("cpuacct")
        return self.Subsystem("cpuacct", self.__pid, self.__owner)

    @property
    def devices(self):
        self._validate_subsystem("devices")
        return self.Subsystem("devices", self.__pid, self.__owner)

    @property
    def freezer(self):
        self._validate_subsystem("freezer")
        return self.Subsystem("freezer", self.__pid, self.__owner)

    @property
    def memory(self):
        self._validate_subsystem("memory")
        return self.Subsystem("memory", self.__pid, self.__owner)

# -*- coding: utf-8 -*-
import os
import pytest
import subprocess

import library.python.cgroups as cgroups


@pytest.fixture
def cgroup():
    cgroup = cgroups.CGroup("test").create()
    yield cgroup
    cgroup.delete()


@pytest.fixture
def pid():
    process = subprocess.Popen(["/bin/sleep", "infinity"], shell=False)
    yield process.pid
    process.terminate()
    process.wait()


def test_cgroup(cgroup):
    assert cgroup.exists


def test_subsystems(cgroup):
    assert cgroup.cpu
    assert cgroup.cpuacct
    assert cgroup.devices
    assert cgroup.freezer
    assert cgroup.freezer.name == "freezer"
    assert cgroup.memory
    assert cgroup["memory"]

    for subsystem in cgroup:  # iterate over all subsystems of the cgroup
        assert subsystem


def test_change_path():
    cgroup = cgroups.CGroup("/custom/path")
    assert cgroup.name == "/custom/path"
    cgroup2 = cgroup >> "subpath"
    assert cgroup2.name == "/custom/path/subpath"
    assert (cgroup << 1).name == "/custom"
    assert (cgroup2 << 2).name == "/custom"
    assert (cgroup.memory >> "subpath").cgroup.name == "/custom/path/subpath"
    assert (cgroup2.cpu << 2).cgroup.name == "/custom"


def test_current_pid():
    # check the occurrence of the process in the cgroup
    cgroup = cgroups.CGroup()
    assert os.getpid() in cgroup  # process there is at least in one of subsystem


def test_add_pid_to_all(cgroup, pid):
    # add process to all subsystems of the cgroup
    cgroup += pid
    assert pid in cgroup
    assert all(pid in subsystem for subsystem in cgroup)


def test_add_pid_to_subsystem(cgroup, pid):
    # add process to specific subsystem of the cgroup
    freezer = cgroup.freezer
    freezer += pid
    assert pid in freezer


def test_limits(cgroup):
    # changing cgroup's limits
    cgroup.memory["limit_in_bytes"] = "11G"
    assert cgroup.memory["limit_in_bytes"] == ["11811160064"]

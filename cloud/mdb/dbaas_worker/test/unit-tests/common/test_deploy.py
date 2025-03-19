# coding: utf-8
from cloud.mdb.dbaas_worker.internal.tasks.common.deploy import BaseDeployExecutor
from test.tasks.utils import get_task_host


def test_is_aws_when_hosts_are_empty_should_return_false():
    assert not BaseDeployExecutor._is_aws({})


def test_is_aws_when_any_host_is_aws_should_return_true():
    assert BaseDeployExecutor._is_aws(
        {'host1': get_task_host(), 'host2': get_task_host(vtype='aws'), 'host3': get_task_host()}
    )


def test_is_aws_when_all_hosts_are_not_aws_should_return_false():
    assert not BaseDeployExecutor._is_aws(
        {'host1': get_task_host(), 'host2': get_task_host(vtype='compute'), 'host3': get_task_host()}
    )


def test_is_compute_when_hosts_are_empty_should_return_false():
    assert not BaseDeployExecutor._is_compute({})


def test_is_compute_when_any_host_is_compute_should_return_true():
    assert BaseDeployExecutor._is_compute(
        {'host1': get_task_host(), 'host2': get_task_host(vtype='compute'), 'host3': get_task_host()}
    )


def test_is_compute_when_all_hosts_are_not_compute_should_return_false():
    assert not BaseDeployExecutor._is_compute(
        {'host1': get_task_host(), 'host2': get_task_host(vtype='aws'), 'host3': get_task_host()}
    )

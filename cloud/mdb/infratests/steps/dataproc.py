import time
import yaml

import humanfriendly
from behave import then, when
from deepdiff import DeepDiff
from hamcrest import assert_that, equal_to, has_entries, is_in

from cloud.mdb.infratests.test_helpers import dataproc, py_api
from cloud.mdb.infratests.test_helpers.compute import get_compute_api
from cloud.mdb.infratests.test_helpers.context import Context
from cloud.mdb.infratests.test_helpers.utils import get_step_data, render_text


@then('all hosts of current cluster have service account {service_account_id}')
def step_check_dataproc_service_account(context: Context, service_account_id: str):
    service_account_id = render_text(context, service_account_id)
    instances = dataproc.get_compute_instances_of_cluster(context)
    for instance in instances:
        got_service_account_id = instance.service_account_id
        assert_that(got_service_account_id, equal_to(service_account_id))


@then('all {hosts_count} hosts of subcluster "{subcluster_name}" have {cores} cores')
def step_check_subcluster_hosts_cores(context: Context, hosts_count: str, subcluster_name: str, cores: str):
    instances = dataproc.get_compute_instances_of_subcluster(context, subcluster_name)
    assert len(instances) == int(
        hosts_count
    ), f"expected subcluster {subcluster_name} to have {hosts_count} hosts, got {len(instances)}"
    for instance in instances:
        try:
            assert_that(instance.resources.cores, equal_to(int(cores)))
        except AttributeError as exception:
            raise AttributeError(f'Instance = {instance}') from exception


@then('all {hosts_count} hosts of subcluster "{subcluster_name}" have {disk_type} boot disk of size {disk_size} GB')
def step_check_subcluster_hosts_disk_size(
    context: Context,
    hosts_count: str,
    subcluster_name: str,
    disk_type: str,
    disk_size: str,
):
    instances = dataproc.get_compute_instances_of_subcluster(context, subcluster_name)
    assert len(instances) == int(
        hosts_count
    ), f"expected subcluster {subcluster_name} to have {hosts_count} hosts, got {len(instances)}"
    compute_api = get_compute_api(context)
    for instance in instances:
        disk = compute_api.get_disk(instance.boot_disk.disk_id)
        assert_that(disk.size, equal_to(int(disk_size) * 2**30))
        assert_that(disk.type_id, equal_to(disk_type))


@then('all {hosts_count} hosts of subcluster "{subcluster_name}" have following labels')
def step_check_subcluster_hosts_labels(context: Context, hosts_count: str, subcluster_name: str):
    instances = dataproc.get_compute_instances_of_subcluster(context, subcluster_name)
    assert len(instances) == int(
        hosts_count
    ), f"expected subcluster {subcluster_name} to have {hosts_count} hosts, got {len(instances)}"
    expected_labels = get_step_data(context)

    for instance in instances:
        labels = instance.labels
        assert (
            labels == expected_labels
        ), f"list of labels for host '{instance.fqdn}' differs from expected: {DeepDiff(labels, expected_labels)}"


@then('user-data attribute of metadata of all hosts of current cluster contains')
def step_check_user_data(context: Context):
    expected_user_data = get_step_data(context)
    instances = dataproc.get_compute_instances_of_cluster(context, view='FULL')
    for instance in instances:
        metadata = instance.metadata
        actual_user_data = yaml.safe_load(metadata['user-data'])['data']
        assert_that(
            actual_user_data,
            has_entries(expected_user_data),
            'Unexpected diff: {diff}'.format(diff=DeepDiff(expected_user_data, actual_user_data)),
        )


@when('we execute "{command}" on master node of Data Proc cluster')
def step_execute_command_on_dataproc_master_node(context: Context, command: str):
    dataproc.execute_command_on_master_node(context, command)


@then('job driver output contains')
def step_job_driver_output_contains(context: Context):
    output, error = py_api.get_dataproc_job_log(context)
    assert error is None, f'Got error when fetched job log: {error}'
    found = context.text in output
    assert found, f'Not found text {context.text} within job driver output. Output is: \n{output}'


@then('job output does not contain')
@then('job driver output does not contain')
def step_job_driver_output_not_contains(context):
    output, error = py_api.get_dataproc_job_log(context)
    assert error is None, f'Got error when fetched job log: {error}'
    missing = context.text not in output
    assert missing, 'Expected job driver\'s output not to contain text {body}'.format(body=context.text)


@when('I submit dataproc job')
def step_submit_dataproc_job(context: Context):
    context.response = py_api.submit_dataproc_job(context, get_step_data(context))
    operation = context.response.json()
    assert str(context.response.status_code) == str(200), 'Failed to submit job: {message}'.format(
        message=operation['message']
    )

    context.operation_id = operation.get('id')
    context.job_id = operation['metadata']['jobId']


@then('we wait until dataproc job is "{statuses}"')
@then('we wait no more than "{timeout}" until dataproc job is "{statuses}"')
def step_wait_job_status(context, statuses, timeout='60s'):
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    statuses = statuses.split(",")
    actual_status = ''
    while time.time() < deadline and actual_status not in statuses:
        time.sleep(1)
        actual_status = py_api.get_job(context, context.job_id)['status']

    if actual_status not in statuses:
        assert_that(
            actual_status,
            is_in(statuses),
            'Timed out waiting until dataproc job status will become equal to expected',
        )

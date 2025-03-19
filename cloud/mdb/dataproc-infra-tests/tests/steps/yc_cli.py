"""
Steps related to YC CLI.
"""
# pylint: disable=too-many-lines

import logging
import os
import re
import shlex
import subprocess
import time

import humanfriendly
import yaml
from behave import given, then, when
from deepdiff import DeepDiff
from hamcrest import assert_that, contains_string, equal_to, has_entries, has_key, is_in

from tests.helpers import internal_api, metadb
from tests.helpers.step_helpers import get_step_data
from tests.helpers.utils import context_to_dict, render_template

TIMEOUT_SECONDS = 1800


@given('we put ssh public keys to {file_path}')
def step_put_ssh_public_keys_to_file(context, file_path):
    cluster_config = context.cluster_config
    with open(file_path, 'w') as outfile:
        outfile.write('\n'.join(cluster_config['configSpec']['hadoop']['sshPublicKeys']) + '\n')


@when('we run YC CLI')
@when('we run YC CLI with "{timeout}" timeout')
@when('we run YC CLI retrying ResourceExhausted exception for "{timeout}"')
@when('we run YC CLI "{runs}" times')
def step_run_yc_cli_job(context, timeout=None, runs=1, full_yc_cli_command=None):
    # Replacing "yc" with a custom yc path if specified
    if not full_yc_cli_command:
        yc_cli_command = render_template(context.text, context_to_dict(context)).replace('\n', ' ').strip()
        yc_cli_command_parts = shlex.split(yc_cli_command)
        _, service, object_type, operation_type = yc_cli_command_parts[:4]
        if yc_cli_command.startswith('yc '):
            yc_cli_command = context.conf['common']['yc_command'] + yc_cli_command[2:]

        yc_infratest_args = ' --format yaml --no-user-output'
        if os.path.exists('staging/yc_config.yaml') and '--config' not in yc_cli_command:
            yc_cli_command += ' --config staging/yc_config.yaml'

        if 'folder_id' in context:
            yc_infratest_args += f' --folder-id {context.folder_id}'
        is_operation_a_job = object_type in ('job', 'jobs', 'tasks')
        if is_operation_a_job:
            cid = context.cluster['id']
            yc_infratest_args += f' --cluster-id {cid}'

        full_yc_cli_command = f'{yc_cli_command} {yc_infratest_args}'

    cmd, out, err = None, None, None
    for _ in range(int(runs)):
        cmd = subprocess.Popen(
            full_yc_cli_command,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            shell=True,
        )
        yc_timeout = int(humanfriendly.parse_timespan(timeout)) if timeout else TIMEOUT_SECONDS
        out, err = cmd.communicate(timeout=yc_timeout)

    context.cli_result = {
        "exit_code": cmd.returncode,
        "command": full_yc_cli_command,
        "stdout": out,
        "stderr": err,
    }

    if cmd.returncode != 0:
        context.operation_id = operation_id_from_cli_error(context.cli_result)
        context.job_id = job_id_from_cli_error(context.cli_result)
        if not timeout:
            return
        assert (
            context.operation_id
        ), f"Failed to extract operation id from yc output: {err.decode('utf-8')}. Command: {full_yc_cli_command}"
        task = internal_api.get_task(context)
        is_not_enough_resources = (task or {}).get('error', {}).get('code') == 8
        if is_not_enough_resources:
            error = "Task not finished until timeout"
            deadline = time.time() + humanfriendly.parse_timespan(timeout)
            finished_successfully = False

            metadb.restart_operation(context)
            while time.time() < deadline:
                task = internal_api.get_task(context)
                assert_that(task, has_key('done'))

                if not task['done']:
                    time.sleep(1)
                    continue
                else:
                    is_not_enough_resources = (task or {}).get('error', {}).get('code') == 8
                    if is_not_enough_resources:
                        metadb.restart_operation(context)
                        time.sleep(10)
                        continue

                logging.debug('Task is done: %r', task)

                finished_successfully = 'error' not in task
                if not finished_successfully:
                    error = task['error']
                break

            assert finished_successfully, '{operation_id} error is {error}'.format(
                operation_id=context.operation_id, error=error
            )


def operation_id_from_cli_error(cli_result):
    error = cli_result['stderr'].decode('utf-8')
    operation_id = None
    for line in error.splitlines():
        match = re.match(r'^operation-id: (.*)', line)
        if match:
            operation_id = match[1]
            break
    return operation_id


def job_id_from_cli_error(cli_result):
    error = cli_result['stderr'].decode('utf-8')
    for line in error.splitlines():
        match = re.match(r'^ERROR: Job (.*) failed', line)
        if match:
            return match[1]


def with_cli_response_details(message, cli_result):
    return (
        '{message}\n'
        'Error message:\n'
        '  {stderr}.\n'
        'Command was:\n'
        '  {command}\n'
        'Stdout: {stdout}'.format(message=message, **cli_result)
    )


@then('YC CLI exit code is {code}')
def step_check_cli_exit_code(context, code):
    cli_result = context.cli_result
    exit_code = cli_result['exit_code']
    assert exit_code == int(code), with_cli_response_details(
        f'YC CLI command exited with unexpected exit code: {exit_code}', cli_result
    )


@then('dataproc job started via YC CLI is {status}')
def step_check_job_status(context, status):
    if context.cli_result['exit_code'] != 0:
        job_id = context.job_id
        if context.operation_id is not None and job_id is None:
            operation = internal_api.get_task(context)
            job_id = operation['metadata']['jobId']
        if job_id is None:
            raise ValueError('Failed to parse job_id')
        job_dict = internal_api.get_job(context, job_id)
    else:
        job_dict = parse_cli_response_yaml(context)

    assert_that(job_dict['status'], equal_to(status))
    context.job_id = job_dict['id']


@then('dataproc job submitted via YC CLI')
def step_get_job_id(context):
    resp_dict = parse_cli_response_yaml(context)
    context.job_id = resp_dict['metadata']['job_id']


@then('dataproc job after another CLI operation started via YC CLI is {status}')
def step_check_async_job_status(context, status):
    job_dict = internal_api.get_job(context, context.job_id)
    assert_that(job_dict['status'], equal_to(status))
    context.job_id = job_dict['id']


@then('we wait until dataproc job is "{statuses}"')
@then('we wait no more than "{timeout}" until dataproc job is "{statuses}"')
def step_wait_job_status(context, statuses, timeout=None):
    if timeout is None:
        timeout = '60s'
    statuses = statuses.split(",")
    deadline = time.time() + humanfriendly.parse_timespan(timeout)
    actual_status = ''
    while time.time() < deadline and actual_status not in statuses:
        time.sleep(1)
        parsed_response = parse_cli_response_yaml(context)
        context.job_id = parsed_response['metadata']['job_id']
        actual_status = internal_api.get_job(context, context.job_id)['status']

    if actual_status not in statuses:
        assert_that(
            actual_status,
            is_in(statuses),
            'Timed out waiting until dataproc job status will become equal to expected',
        )


def parse_cli_response_yaml(context):
    cli_result = context.cli_result
    try:
        return yaml.safe_load(cli_result['stdout'])
    except yaml.YAMLError:
        assert False, with_cli_response_details('YC CLI command returned non-YAML response', cli_result)


@then('periodically run previous YC CLI command for "{timeout}" until response contains')
@then('periodically run previous YC CLI command for "{timeout}" until response key "{key}" contains')
def poll_cli(context, timeout, key=None):
    timeout_seconds = humanfriendly.parse_timespan(timeout)
    deadline = time.time() + timeout_seconds
    if not context.text:
        raise ValueError('No expected result result is provided')
    retry_interval_seconds = 3

    response = None
    expected_result = None
    while time.time() < deadline:
        expected_result = get_step_data(context)
        response = parse_cli_response_yaml(context)
        if key:
            response = response[key]
        try:
            assert_that(
                response,
                has_entries(expected_result),
                'Actual result: {actual_result}\nUnexpected diff: {diff}'.format(
                    actual_result=yaml.safe_dump(response), diff=DeepDiff(expected_result, response)
                ),
            )
            break
        except AssertionError:
            time.sleep(retry_interval_seconds)
            if timeout_seconds > retry_interval_seconds:
                timeout_seconds -= retry_interval_seconds
            step_run_yc_cli_job(
                context,
                timeout=str(timeout_seconds),
                full_yc_cli_command=context.cli_result['command'],
            )
    assert_that(
        response,
        has_entries(expected_result),
        'Actual result: {actual_result}\nUnexpected diff: {diff}'.format(
            actual_result=yaml.safe_dump(response), diff=DeepDiff(expected_result, response)
        ),
    )


@then('YC CLI response contains')
@then('YC CLI response key "{key}" contains')
def step_check_cli_result(context, key=None):
    if context.text:
        expected_result = get_step_data(context)
        response = parse_cli_response_yaml(context)
        if isinstance(response, list):
            response = response[0]
        if key:
            response = response[key]
        assert_that(
            response,
            has_entries(expected_result),
            'Actual result: {actial_result}\nUnexpected diff: {diff}'.format(
                actial_result=yaml.safe_dump(response), diff=DeepDiff(expected_result, response)
            ),
        )

    else:
        raise ValueError('No expected result result is provided')


@then('YC CLI error contains')
def step_check_cli_error(context):
    if context.text:
        context_text = render_template(context.text, context_to_dict(context)).replace('\n', ' ').strip()
        response = context.cli_result['stderr']
        assert_that(str(response), contains_string(context_text))
    else:
        raise ValueError('No expected error is provided')


@when('save YC CLI response as {var_name} var')
def step_save_cli_response_to_var(context, var_name):
    response = parse_cli_response_yaml(context)
    setattr(context, var_name, response)

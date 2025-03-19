"""
Steps related to YC CLI.
"""
# pylint: disable=too-many-lines

import re
import shlex
import subprocess
import yaml

import humanfriendly
from behave import then, when
from deepdiff import DeepDiff
from hamcrest import assert_that, contains_string, equal_to, has_entries
from typing import Optional

from cloud.mdb.infratests.test_helpers import py_api
from cloud.mdb.infratests.test_helpers.context import Context
from cloud.mdb.infratests.test_helpers.types import CliResult
from cloud.mdb.infratests.test_helpers.utils import context_to_dict, get_step_data, render_template

TIMEOUT_SECONDS = 1800


@when('we run YC CLI')
@when('we run YC CLI with "{timeout}" timeout')
@when('we run YC CLI "{runs}" times')
def step_run_yc_cli_job(context: Context, timeout: str = None, runs: str = 1, full_yc_cli_command: str = None):
    is_operation_a_job = False
    if not full_yc_cli_command:
        yc_cli_command = render_template(context.text, context_to_dict(context)).replace('\n', ' ').strip()
        yc_cli_command_parts = shlex.split(yc_cli_command)
        _, service, object_type, operation_type = yc_cli_command_parts[:4]
        yc_cli_command = context.test_config.yc_bin_path + yc_cli_command[2:]
        yc_infratest_args = ' --format yaml --no-user-output'
        yc_cli_command += f' --config {context.test_config.yc_config_path}'
        is_operation_a_job = object_type in ('job', 'jobs', 'tasks')
        if is_operation_a_job:
            yc_infratest_args += f' --cluster-id {context.cid}'

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

    context.cli_result = CliResult(
        exit_code=cmd.returncode,
        command=full_yc_cli_command,
        stdout=out,
        stderr=err,
    )

    if cmd.returncode == 0:
        if is_operation_a_job and '--async' in full_yc_cli_command:
            parsed_response = _parse_cli_response_yaml(context)
            context.job_id = parsed_response['metadata']['job_id']
    else:
        job_id = _job_id_from_cli_error(context.cli_result)
        if job_id:
            context.job_id = job_id
        else:
            context.operation_id = _operation_id_from_cli_error(context.cli_result)
            assert (
                context.operation_id
            ), f"Failed to extract operation id from yc output: {err.decode('utf-8')}. Command: {full_yc_cli_command}"


@then('YC CLI error contains')
def step_check_cli_error(context: Context):
    assert context.text, 'No expected error is provided'
    context_text = get_step_data(context, format='text').replace('\n', ' ').strip()
    response = context.cli_result.stderr
    assert_that(str(response), contains_string(context_text))


@then('YC CLI exit code is {code}')
def step_check_cli_exit_code(context: Context, code: str):
    cli_result = context.cli_result
    exit_code = cli_result.exit_code
    assert exit_code == int(code), _with_cli_response_details(
        f'YC CLI command exited with unexpected exit code: {exit_code}', cli_result
    )


@then('YC CLI response contains')
@then('YC CLI response key "{key}" contains')
def step_check_cli_result(context: Context, key: str = None):
    if context.text:
        expected_result = get_step_data(context)
        response = _parse_cli_response_yaml(context)
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


@then('dataproc job started via YC CLI is {status}')
def step_check_job_status(context: Context, status: str):
    if context.cli_result.exit_code != 0:
        job_id = context.job_id
        if not job_id:
            operation = py_api.get_task(context)
            job_id = operation['metadata']['jobId']
        if job_id is None:
            raise ValueError('Failed to parse job_id')
        job_dict = py_api.get_job(context, job_id)
    else:
        job_dict = _parse_cli_response_yaml(context)

    assert_that(job_dict['status'], equal_to(status))
    context.job_id = job_dict['id']


@when('save YC CLI response as {var_name} var')
def step_save_cli_response_to_var(context: Context, var_name: str):
    response = _parse_cli_response_yaml(context)
    setattr(context, var_name, response)


def _job_id_from_cli_error(cli_result: CliResult):
    error = cli_result.stderr.decode('utf-8')
    for line in error.splitlines():
        match = re.match(r'^ERROR: Job (.*) failed', line)
        if match:
            return match[1]


def _operation_id_from_cli_error(cli_result: CliResult) -> Optional[str]:
    error = cli_result.stderr.decode('utf-8')
    operation_id = None
    for line in error.splitlines():
        match = re.match(r'^operation-id: (.*)', line)
        if match:
            operation_id = match[1]
            break
    return operation_id


def _parse_cli_response_yaml(context: Context):
    cli_result = context.cli_result
    try:
        return yaml.safe_load(cli_result.stdout)
    except yaml.YAMLError:
        assert False, _with_cli_response_details('YC CLI command returned non-YAML response', cli_result)


def _with_cli_response_details(message: str, cli_result: CliResult):
    return (
        f'{message}\n'
        'Error message:\n'
        f'  {cli_result.stderr}.\n'
        'Command was:\n'
        f'  {cli_result.command}\n'
        f'Stdout: {cli_result.stdout}'
    )

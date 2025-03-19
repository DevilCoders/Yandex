import pathlib
import subprocess
from typing import Tuple, Optional
from unittest.mock import patch, ANY

import pytest
from pytest_subprocess import FakeProcess

import agent.process
import agent.process_buffer
from agent.config.models import BashProberRunner, ProberConfig, ClusterMetadata, Cluster, Prober
from agent.status import ProberExecutionStatus
from database.models import UploadProberLogPolicy
from common.monitoring.solomon import SolomonClient

TEST_READ_CHUNK_SIZE = 1024 * 256  # 256 KiB
TEST_BUFFER_SIZE = 1024 * 1024  # 1 MiB

cluster = ClusterMetadata(
    Cluster(
        id=1,
        name="Meeseeks",
        slug="meeseeks",
        variables={},
        prober_configs=[],
    )
)

success_prober = Prober(
    id=1,
    name="Success",
    slug="success",
    runner=BashProberRunner(
        command="echo Success",
    ),
    files=[],
    files_location=pathlib.Path("/tmp"),
)

bash_substitution_prober = Prober(
    id=1,
    name="Bash Substitution",
    slug="bash_substitution",
    runner=BashProberRunner(
        command="echo $((1 + 2))",
    ),
    files=[],
    files_location=pathlib.Path("/tmp"),
)
testdata_prober_config_variables = {
    "none_var": None,
    "str_var_1": "str_value",
    "str_var_2": "str_value 2",
    "int_var": 1,
    "float_var": 2.54,
    "bool_var": True,
    "dict_var": {"sub_key_1": "value1", "sub_key_2": 2},
    "list_var": ["value1", 2],
    "tuple_var": ("value1", 2),
}
testdata_prober_config = ProberConfig(
    id=1,
    prober_id=bash_substitution_prober.id,
    is_prober_enabled=True,
    interval_seconds=5,
    timeout_seconds=3,
    s3_logs_policy=UploadProberLogPolicy.FAIL,
    variables=testdata_prober_config_variables,
)
testdata_stdout = b"stdout"
testdata_stderr = b"stderr"
testdata_big_text = b"1" * 1024 * 1024 * 2  # 2 MiB


def test_create_prober_process_with_bash_prober_runner():
    """
    Checks that prober with BashProberRunner created subprocess.Popen()
    """
    prober_config = ProberConfig(
        prober_id=success_prober.id,
        is_prober_enabled=True,
        interval_seconds=5,
        timeout_seconds=3,
        s3_logs_policy=UploadProberLogPolicy.FAIL,
    )

    process = agent.process.ProberProcess(success_prober, prober_config, cluster, SolomonClient())
    with patch.object(subprocess, "Popen") as Popen:
        process.start()

    Popen.assert_called_once_with(
        "echo Success",
        cwd="/tmp",
        shell=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        env={},
        executable="/bin/bash",
        start_new_session=True,
        preexec_fn=ANY,
    )


def test_bash_prober_runner_works_with_bash_substitution():
    """
    Checks that BashProberRunner runs process with shell=True, it allows to use commands like
        ./metadata.sh network/interfaces/macs/$(ifconfig eth0 | grep -oP "(?<=ether )\S+")/local-hostname $(hostname -s).ru-central1.internal'
    """
    process = bash_substitution_prober.runner.create_process(bash_substitution_prober, testdata_prober_config)
    stdout, stderr = process.communicate()
    assert stdout == b"3\n"


@pytest.mark.parametrize(
    "var_name,expected_output",
    [
        ("not_exists_var", b""),
        ("none_var", b"null"),
        ("str_var_1", b"str_value"),
        ("str_var_2", b"\"str_value 2\""),
        ("int_var", b"1"),
        ("float_var", b"2.54"),
        ("bool_var", b"true"),
        ("dict_var", b"{\"sub_key_1\": \"value1\", \"sub_key_2\": 2}"),
        ("list_var", b"[\"value1\", 2]"),
        ("tuple_var", b"[\"value1\", 2]"),
    ]
)
def test_bash_prober_runner_works_with_variables_substitution(var_name, expected_output):
    prober = Prober(
        id=1,
        name="Bash Prober Variable Substitution",
        slug="bash_prober_variable_substitution",
        runner=BashProberRunner(
            command=f"echo -n ${{VAR_{var_name}}}",
        ),
        files=[],
        files_location=pathlib.Path("/tmp"),
    )
    process = prober.runner.create_process(prober, testdata_prober_config)
    stdout, stderr = process.communicate()
    assert stdout == expected_output


def _get_default_path_environment() -> str:
    return subprocess.check_output("env -i bash -c 'echo $PATH'", shell=True).decode().strip()


@pytest.mark.parametrize(
    "command,expected_output",
    [
        ("echo $HOME", ""),
        ("echo $PATH", _get_default_path_environment()),
    ],
    ids=("$HOME", "$PATH")
)
def test_bash_process_runner_has_empty_environment(command: str, expected_output: str):
    env_runner = BashProberRunner(**bash_substitution_prober.runner.dict())
    env_runner.command = command
    process = env_runner.create_process(bash_substitution_prober, testdata_prober_config)
    stdout, stderr = process.communicate()
    assert stdout.decode().strip() == expected_output


@pytest.fixture
def mocked_select():
    """
    Fixture to mock `select.select()`. Return all element are ready-to-read/write
    """
    with patch("select.select", side_effect=lambda r, w, x, *args: [r, w, x]):
        yield


@pytest.mark.parametrize(
    "poll_results,expected_exit_code,expected_status,expected_stdout,expected_stderr",
    [
        (
                (0,), 0, ProberExecutionStatus.SUCCESS, testdata_stdout, testdata_stderr
        ),
        (
                (10,), 10, ProberExecutionStatus.FAIL, testdata_stdout, testdata_stderr
        ),
        (
                (None, 0), 0, ProberExecutionStatus.SUCCESS, testdata_stdout, testdata_stderr
        ),
        (
                (None, None, None, 0), None, ProberExecutionStatus.TIMEOUT, testdata_stdout, testdata_stderr
        ),
    ],
    ids=("success", "fail", "success-on-second-iteration", "timeout")
)
def test_prober_process_check_status_on_finished_process(
    poll_results: Tuple[Optional[int]],
    expected_exit_code: int,
    expected_status: ProberExecutionStatus,
    expected_stdout: bytes,
    expected_stderr: bytes,
    fake_process: FakeProcess,
    mocked_select,
):
    prober_config = ProberConfig(
        prober_id=success_prober.id,
        is_prober_enabled=True,
        interval_seconds=5,
        timeout_seconds=2,
        s3_logs_policy=UploadProberLogPolicy.FAIL,
    )

    fake_process.register_subprocess(
        "echo Success", stdout=testdata_stdout, stderr=testdata_stderr, returncode=expected_exit_code,
    )

    # Each iteration "works" for 1 second in this test. So if prober fits in first, second or third iterations,
    # it will not fail with timeout. On third iteration total elapsed time is 3 seconds
    # (timeout_seconds is 2 — see above) and process has not finished yet.
    # In this case prober execution should finish with ProberExecutionStatus.TIMEOUT
    iterations_count = len(poll_results)
    iteration_duration_seconds = 1
    timer_total_seconds_results = (
        (iteration + 1) * iteration_duration_seconds for iteration in range(iterations_count))

    with patch.object(agent.process, "ProberTelemetrySender") as ProberTelemetrySender, \
            patch.object(agent.process, "Timer") as TimerMock, \
            patch.object(TimerMock.return_value, "get_total_seconds", side_effect=timer_total_seconds_results):
        process = agent.process.ProberProcess(success_prober, prober_config, cluster, SolomonClient())
        process.start()
        with patch.object(process._process, "poll", side_effect=poll_results), \
                patch.object(process, "_kill", new=lambda self: self._process.kill()), \
                patch.object(
                    ProberTelemetrySender.return_value, "send_telemetry_about_prober_execution"
                ) as send_telemetry:
            for _ in range(len(poll_results)):
                process.check_status()

            send_telemetry.assert_called_once_with(
                ANY, iteration_duration_seconds * iterations_count, expected_status, expected_exit_code, ANY, ANY
            )

        assert process.stdout.get_data() == expected_stdout
        assert process.stderr.get_data() == expected_stderr


def test_prober_process_with_delay():
    """
    Checks that ProberProcess understands delay_seconds and can waits until the first start
    """
    prober_config = ProberConfig(
        prober_id=success_prober.id,
        is_prober_enabled=True,
        interval_seconds=5,
        timeout_seconds=3,
        s3_logs_policy=UploadProberLogPolicy.FAIL,
    )

    # Let's start at 100 seconds with delay 10 seconds
    with patch("time.monotonic") as monotonic:
        monotonic.return_value = 100
        process = agent.process.ProberProcess(success_prober, prober_config, cluster, SolomonClient(), delay_seconds=10)
        assert not process.need_start()

        # In the next moment (105 seconds) we are not ready to start yet
        monotonic.return_value = 105
        assert not process.need_start()

        # as in the next moment (109 seconds)
        monotonic.return_value = 109
        assert not process.need_start()

        # We're ready to start only at 110 seconds
        monotonic.return_value = 110
        assert process.need_start()


def test_prober_process_with_delay_and_interval(fake_process: FakeProcess, mocked_select):
    """
    Checks that ProberProcess waits `delay_seconds` only one time, at it's start
    """
    prober_config = ProberConfig(
        prober_id=success_prober.id,
        is_prober_enabled=True,
        interval_seconds=5,
        timeout_seconds=3,
        s3_logs_policy=UploadProberLogPolicy.NONE,
    )

    fake_process.register_subprocess("echo Success", stdout=b"Success")

    with patch("time.monotonic") as monotonic:
        monotonic.return_value = 100
        process = agent.process.ProberProcess(success_prober, prober_config, cluster, SolomonClient(), delay_seconds=10)

        # Start the process for the first time after 10 seconds
        monotonic.return_value = 110
        assert process.need_start()
        process.start()
        assert not process.need_start()

        with patch.object(agent.process, "ProberTelemetrySender"):
            process.check_status()

        # Second start should be scheduled after 5 seconds (`interval_second`)
        monotonic.return_value = 115.01
        assert process.need_start()

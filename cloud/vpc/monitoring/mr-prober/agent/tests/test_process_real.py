import pathlib
import tempfile
import time
from unittest.mock import MagicMock

import pytest

from agent.config.models import ProberConfig, Prober, Cluster, ClusterMetadata, BashProberRunner, UploadProberLogPolicy
from agent.process import ProberProcess
from agent.tests.test_agent import DummySolomonClient

pytestmark = pytest.mark.integration


def test_prober_process_reads_output():
    prober = Prober(
        id=1,
        name="Bash Prober",
        slug="bash_prober",
        runner=BashProberRunner(command="true"),
        files=[],
        files_location=pathlib.Path("/tmp"),
    )
    prober_config = ProberConfig(
        id=1,
        prober_id=prober.id,
        is_prober_enabled=True,
        interval_seconds=5,
        timeout_seconds=10,
        s3_logs_policy=UploadProberLogPolicy.FAIL
    )
    cluster_metadata = ClusterMetadata(
        Cluster(
            id=1,
            name="Test cluster",
            slug="test_cluster",
            variables={},
            prober_configs=[],
        )
    )

    # check command without output
    prober_process = start_process_and_wait_it(prober, prober_config, cluster_metadata)
    assert prober_process.stdout.get_data() == b""
    assert prober_process.stderr.get_data() == b""

    # check command with an empty output
    prober.runner.command = "echo -n \"\"; echo -n \"\" >&2"
    prober_process = start_process_and_wait_it(prober, prober_config, cluster_metadata)
    assert prober_process.stdout.get_data() == b""
    assert prober_process.stderr.get_data() == b""

    # check command with a small output
    prober.runner.command = "echo -n 1234567890; echo -n 0987654321 >&2"
    prober_process = start_process_and_wait_it(prober, prober_config, cluster_metadata)
    assert prober_process.stdout.get_data() == b"1234567890"
    assert prober_process.stderr.get_data() == b"0987654321"

    # check command with an average output size
    with tempfile.NamedTemporaryFile(mode="w", prefix="mr-prober-process-tests") as test_file:
        test_file.write("1" * 1024 * 1024)  # 1 MiB
        test_file.flush()
        prober.runner.command = f"cat {test_file.name}"
        prober_process = start_process_and_wait_it(prober, prober_config, cluster_metadata)

        assert prober_process.stdout.get_data() == b"1" * 1024 * 1024
        assert prober_process.stderr.get_data() == b""

    # check command with an output size exceeding the cache size
    with tempfile.NamedTemporaryFile(mode="w", prefix="mr-prober-process-tests") as test_file:
        test_file.write("1" * 1024 * 1024 * 12)  # 12 MiB
        test_file.flush()
        prober.runner.command = f"cat {test_file.name}"
        prober_process = start_process_and_wait_it(prober, prober_config, cluster_metadata)

        assert prober_process.stdout.total_read == prober_process.stdout.BUFFER_SIZE
        assert prober_process.stdout.get_data() == b"1" * prober_process.stdout.BUFFER_SIZE
        assert prober_process.stderr.get_data() == b""


def start_process_and_wait_it(prober: Prober, prober_config: ProberConfig, cluster_metadata: ClusterMetadata):
    prober_process = ProberProcess(prober, prober_config, cluster_metadata, DummySolomonClient())
    prober_process._telemetry_sender = MagicMock()
    prober_process.start()
    iterations_count = 1000
    for i in range(iterations_count):
        prober_process.check_status()
        if prober_process._process is None:
            break
        time.sleep(0.01)
    else:
        pytest.fail(
            "the process did not have time to complete in the allotted number of cycles. "
            "It may be worth increasing the iterations_count"
        )
    return prober_process

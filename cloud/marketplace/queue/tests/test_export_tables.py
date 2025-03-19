import pytest

from cloud.marketplace.common.yc_marketplace_common.lib import TaskUtils
from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.tasks import export_tables


@pytest.mark.parametrize(
    ["params", "expected_status"],
    [
        (
            {},
            TaskResolution.Status.FAILED,
        ),
        (
            {"export_destination": "test"},
            TaskResolution.Status.FAILED,
        ),
    ],
)
def test_export_tables_error(mocker, monkeypatch, params, expected_status):
    mocker.patch.object(TaskUtils, "create")

    expected_result = export_tables({"params": params})

    assert isinstance(expected_result, TaskResolution)
    assert hasattr(expected_result, "error")
    assert expected_result.error.message == expected_status


@pytest.mark.parametrize(
    ["params"],
    [
        (
            {"export_destination": "logbroker"},
        ),
    ],
)
def test_export_tables(mocker, monkeypatch, params):
    mocker.patch.object(TaskUtils, "create")

    expected_result = export_tables({"params": params})

    assert isinstance(expected_result, TaskResolution)
    assert hasattr(expected_result, "result")
    assert expected_result.result is None

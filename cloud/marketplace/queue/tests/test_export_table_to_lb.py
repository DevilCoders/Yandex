import pytest

from cloud.marketplace.common.yc_marketplace_common.models.task import TaskResolution
from cloud.marketplace.queue.yc_marketplace_queue.tasks import export_table_to_lb
# from cloud.marketplace.queue.yc_marketplace_queue.tasks.export_table_to_lb import TvmClient
from cloud.marketplace.queue.yc_marketplace_queue.tasks.export_table_to_lb import TVMCredentialsProvider


@pytest.mark.parametrize(
    ["params", "expected_status"],
    [
        (
            {},
            TaskResolution.Status.FAILED,
        ),
        (
            {"table_name": "test"},
            TaskResolution.Status.FAILED,
        ),
    ],
)
def test_export_table_to_lb_error(mocker, params, expected_status):
    mocker.patch("cloud.billing.lb_exporter.core.export")
    mocker.patch("kikimr.public.sdk.python.persqueue.auth.TVMCredentialsProvider")

    expected_result = export_table_to_lb({"params": params})

    assert isinstance(expected_result, TaskResolution)
    assert hasattr(expected_result, "error")
    assert expected_result.error.message == expected_status


@pytest.mark.parametrize(
    ["params"],
    [
        (
            {"table_name": "i18n"},
        ),
    ],
)
def test_export_table_to_lb(mocker, monkeypatch, params):
    mocker.patch("cloud.billing.lb_exporter.core.export")
    mocker.patch("ticket_parser2.TvmClient")
    mocker.patch("ticket_parser2.TvmApiClientSettings")
    monkeypatch.setattr(TVMCredentialsProvider, "__init__", lambda *args, **kwargs: None)

    expected_result = export_table_to_lb({"params": params})
    assert isinstance(expected_result, TaskResolution)
    assert hasattr(expected_result, "result")
    assert expected_result.result is None

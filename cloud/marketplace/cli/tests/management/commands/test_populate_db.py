import pytest

from yc_common.exceptions import Error
from cloud.marketplace.cli.yc_marketplace_cli.commands.populate_db import Command


@pytest.mark.parametrize(
    ["func_kwargs", "is_error", "fill_kikimr_calls", "fill_queue_tasks_calls"],
    [
        ({"erase": True, "devel": False, "fill_all": True, "fill_tasks": True}, True, None, None),
        ({"erase": True, "devel": True, "fill_all": True, "fill_tasks": True}, False, 1, 1),
        ({"erase": True, "devel": True, "fill_all": False, "fill_tasks": True}, False, 0, 1),
        ({"erase": True, "devel": True, "fill_all": True, "fill_tasks": False}, False, 1, 1),
        ({"erase": True, "devel": True, "fill_all": False, "fill_tasks": False}, False, 0, 0),
    ],
)
def test_handle(mocker, func_kwargs, is_error, fill_kikimr_calls, fill_queue_tasks_calls):
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.populate_db.config")
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.populate_db.marketplace_db")
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.populate_db.DB_LIST_BY_NAME")
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.populate_db.Category")
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.populate_db.MigrationsPropagate")
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.populate_db.TaskUtils")

    if is_error:
        with pytest.raises(Error):
            Command().handle(**func_kwargs)
    else:
        mocker.spy(Command, "fill_kikimr")
        mocker.spy(Command, "fill_queue_tasks")

        result = Command().handle(**func_kwargs)

        assert result is None
        assert Command.fill_kikimr.call_count == fill_kikimr_calls
        assert Command.fill_queue_tasks.call_count == fill_queue_tasks_calls

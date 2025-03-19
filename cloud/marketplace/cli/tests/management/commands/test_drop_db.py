import pytest

from yc_common.exceptions import Error
from cloud.marketplace.cli.yc_marketplace_cli.commands.drop_db import Command


@pytest.mark.parametrize(
    ["devel", "is_error"],
    [
        (True, False),
        (False, True),
    ],
)
def test_handle(mocker, devel, is_error):
    mocker.patch.object(Command, "_drop_directory_recursively")
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.drop_db.config")

    if is_error:
        with pytest.raises(Error):
            Command().handle(devel)
    else:
        result = Command().handle(devel)

        assert result is None


@pytest.mark.parametrize(
    ["list_directory_side", "func_calls"],
    [
        ([[{"directory": False, "name": "test"}]], 1),
        ([[{"directory": True, "name": "test"}], [{"directory": False, "name": "test"}]], 2),
    ],
)
def test__drop_directory_recursively(mocker, list_directory_side, func_calls):
    client_mock = mocker.MagicMock()
    client_mock.list_directory.side_effect = list_directory_side
    mocker.patch("cloud.marketplace.cli.yc_marketplace_cli.commands.drop_db.marketplace_db", return_value=client_mock)

    mocker.spy(Command, "_drop_directory_recursively")
    result = Command._drop_directory_recursively("test")

    assert result is None
    assert Command._drop_directory_recursively.call_count == func_calls

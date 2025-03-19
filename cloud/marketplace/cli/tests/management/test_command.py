import pytest

from cloud.marketplace.cli.yc_marketplace_cli.command import BaseCommand


class Command(BaseCommand):
    def handle(self, **kwargs):
        pass


@pytest.mark.parametrize(
    ["argv", "options_dict"],
    [
        ([], {}),
        (["test=1"], {"test": 1}),
    ],
)
def test_execute(mocker, argv, options_dict):
    mocker.patch.object(Command, "_create_parser")
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.management.command.vars", return_value=options_dict)

    mocker.patch.object(Command, "set_environment")
    mocker.patch.object(Command, "handle")
    mocker.spy(Command, "handle")

    result = Command.execute(argv)

    assert result is None
    Command.handle.assert_called_once_with(**options_dict)


@pytest.mark.parametrize(
    ["func_kwargs", "config_path_res"],
    [
        ({"devel": True, "config_path": "test"}, "test"),
        ({"devel": False, "config_path": "test"}, "test"),
        ({"devel": True, "config_path": None}, "dev_path"),
        ({"devel": False, "config_path": None}, "default_path"),
    ],
)
def test_set_environment(mocker, func_kwargs, config_path_res):
    mocker.patch("importlib.util.find_spec")

    app_mock = mocker.MagicMock()
    app_mock.__file__ = None
    mocker.patch("importlib.util.module_from_spec", return_value=app_mock)

    dev_path = "dev_path"
    mocker.patch("cloud.marketplace.common.yc_marketplace_common.management.command.dev_config", return_value=dev_path)

    mocker.patch("cloud.marketplace.common.yc_marketplace_common.management.command.logging")
    os_mock = mocker.patch("cloud.marketplace.common.yc_marketplace_common.management.command.os")

    mocker.patch.object(Command, "CONFIG_PATH", "default_path")
    config_mock = mocker.patch("cloud.marketplace.common.yc_marketplace_common.management.command.config")
    mocker.spy(config_mock, "load")

    result = Command.set_environment(verbosity=1, **func_kwargs)

    config_load_args = [[config_path_res, os_mock.path.join.return_value]]

    assert result is None
    config_mock.load.assert_called_once_with(*config_load_args, ignore_unknown=True)

import argparse

import pytest
import unittest.mock as m

from cloud.marketplace.cli.yc_marketplace_cli.core import ManagementCommand


@pytest.mark.parametrize(
    ["apps", "get_subcommands_calls", "get_subcommands_side", "subcommands_result"],
    [
        (
            [],
            [],
            [],
            {},
        ),
        (
            ["test_app", "test_app2"],
            [m.call("test_app"), m.call("test_app2")],
            [[], []],
            {},
        ),
        (
            ["test_app", "test_app2"],
            [m.call("test_app"), m.call("test_app2")],
            [[], ["c1"]],
            {"test_app2": {"c1"}},
        ),
        (
            ["test_app", "test_app2"],
            [m.call("test_app"), m.call("test_app2")],
            [["c1", "c2"], ["c3"]],
            {"test_app": {"c1", "c2"}, "test_app2": {"c3"}},
        ),
        (
            ["test_app", None],
            [m.call("test_app")],
            [["c1", "c2"], None],
            {"test_app": {"c1", "c2"}},
        ),
    ],
)
def test_manage_init(mocker, apps, get_subcommands_calls, get_subcommands_side, subcommands_result):
    mocker.patch("importlib.util.find_spec", side_effect=apps)
    mocker.patch("importlib.util.module_from_spec", side_effect=apps)
    mocker.patch.object(ManagementCommand, "APPS", apps)
    mocker.patch.object(ManagementCommand, "get_subcommands", side_effect=get_subcommands_side)
    get_subcommands = mocker.spy(ManagementCommand, "get_subcommands")

    result = ManagementCommand()

    assert isinstance(result, ManagementCommand)
    assert get_subcommands.mock_calls == get_subcommands_calls
    assert result.subcommands == subcommands_result


@pytest.mark.parametrize(
    ["path_exists", "iter_modules", "expected_result"],
    [
        (False, [("", "c1", False)], []),
        (True, [], []),
        (True, [("", "c1", False), ("", "c2", True), ("", "c3", False)], ["c1", "c3"]),
    ],
)
def test_manage_get_subcommands(mocker, path_exists, iter_modules, expected_result):
    os_mock = mocker.patch("cloud.marketplace.common.yc_marketplace_common.management.core.os")
    os_mock.path.exists.return_value = path_exists

    mocker.patch("pkgutil.iter_modules", return_value=iter_modules)

    app_mock = mocker.MagicMock()
    app_mock.__file__ = None
    result = ManagementCommand.get_subcommands(app_mock)

    assert result == expected_result


@pytest.mark.parametrize(
    ["argv", "get_subcommad_calls", "subcommad_execute_calls"],
    [
        ([], [], []),
        (["test"], [], []),
        (["test", "test2", "test3"], ["test2"], [["test3"]]),
        (["test", "test2", "test3", "test4"], ["test2"], [["test3", "test4"]]),
    ],
)
def test_manage_execute(mocker, argv, get_subcommad_calls, subcommad_execute_calls):

    mocker.patch.object(ManagementCommand, "__init__", return_value=None)

    subcommand = mocker.patch.object(ManagementCommand, "validate_subcommand")
    mocker.spy(ManagementCommand, "validate_subcommand")
    mocker.spy(subcommand.return_value, "execute")

    result = ManagementCommand.execute(argv)

    assert result is None
    assert ManagementCommand.validate_subcommand.mock_calls == list(map(lambda x: m.call(x), get_subcommad_calls))
    assert subcommand.return_value.execute.mock_calls == list(map(lambda x: m.call(x), subcommad_execute_calls))


@pytest.mark.parametrize(
    ["subcommand", "subcommands", "is_error"],
    [
        ("test", {}, True),
        ("test", {"app": {"bad"}}, True),
        ("test", {"app": {"bad", "test"}}, False),
    ],
)
def test_validate_subcommand(mocker, subcommand, subcommands, is_error):
    mocker.patch("importlib.util.find_spec")
    module_from_spec = mocker.patch("importlib.util.module_from_spec")
    mocker.patch.object(ManagementCommand, "__init__", return_value=None)

    command_mock = mocker.MagicMock()
    module_from_spec.return_value.Command = command_mock
    management = ManagementCommand()
    mocker.patch.object(management, "subcommands", subcommands)

    if is_error:
        with pytest.raises(argparse.ArgumentError):
            management.validate_subcommand(subcommand)
    else:
        result = management.validate_subcommand(subcommand)

        assert result is command_mock

import json
from collections import OrderedDict
from pathlib import Path
from typing import Any, Type, List, Dict, Optional
from unittest.mock import patch, call

import pytest

from agent.config import S3AgentConfigLoader
from agent.config.s3.downloader import AgentConfigS3Downloader
from agent.config.models import Cluster, ProberConfig, Prober, ProberFile, BashProberRunner
from database.models import UploadProberLogPolicy


def test_agent_config_loader():
    cluster = Cluster(
        id=1,
        name='meeseeks',
        slug='meeseeks',
        variables={},
        prober_configs=[
            ProberConfig(
                prober_id=1,
                hosts_re='localhost',
                is_prober_enabled=True,
                interval_seconds=10,
                timeout_seconds=10,
                s3_logs_policy=UploadProberLogPolicy.FAIL
            ),
            ProberConfig(
                prober_id=2,
                hosts_re='localhost',
                is_prober_enabled=True,
                interval_seconds=10,
                timeout_seconds=10,
                s3_logs_policy=UploadProberLogPolicy.ALL
            )
        ]
    )

    probers = (
        Prober(
            id=1,
            name="External DNS request",
            slug="dns-external",
            runner=BashProberRunner(
                command="dig yandex.ru",
            ),
            files=[]
        ),
        Prober(
            id=2,
            name="External HTTPS request",
            slug="https-external",
            runner=BashProberRunner(
                command="curl https://yandex.ru",
            ),
            files=[
                ProberFile(
                    id=1,
                    relative_file_path="curl.sh",
                    is_executable=True,
                    md5_hexdigest="a2e14daf695d4b76eb60ed9c91f450fd",
                ),
                ProberFile(
                    id=2,
                    relative_file_path="dns.sh",
                    is_executable=False,
                    md5_hexdigest="b2e14daf695d4b76eb60ed9c91f450fd",
                )
            ]
        )
    )

    with patch.object(AgentConfigS3Downloader, "_get_cluster", return_value=(cluster, False)), \
            patch.object(AgentConfigS3Downloader, "_get_prober", side_effect=((p, False) for p in probers)), \
            patch.object(AgentConfigS3Downloader, "_is_matched_with_hosts_re", return_value=True), \
            patch.object(AgentConfigS3Downloader, "download_and_store_prober_file") as download_prober_file_mock, \
            patch.object(Path, "chmod") as chmod_mock, \
            patch.object(Path, "stat"):
        config = S3AgentConfigLoader().load()
        download_prober_file_mock.assert_called()
        chmod_mock.assert_has_calls(
            [
                call(0o755),
                call(0o644)
            ]
        )

    assert len(config.probers) == 2

    for idx, prober_with_config in enumerate(config.probers):
        assert prober_with_config.prober.id == probers[idx].id
        assert prober_with_config.prober.name == probers[idx].name
        assert prober_with_config.prober.slug == probers[idx].slug
        assert prober_with_config.prober.runner == probers[idx].runner
        assert prober_with_config.config.interval_seconds == cluster.prober_configs[idx].interval_seconds
        assert prober_with_config.config.timeout_seconds == cluster.prober_configs[idx].timeout_seconds
        assert prober_with_config.config.s3_logs_policy == cluster.prober_configs[idx].s3_logs_policy

    prober_with_config = config.probers[1]
    for idx, file in enumerate(prober_with_config.prober.files):
        assert file.id == probers[1].files[idx].id
        assert file.is_executable == probers[1].files[idx].is_executable
        assert file.relative_file_path == probers[1].files[idx].relative_file_path
        assert file.md5_hexdigest == probers[1].files[idx].md5_hexdigest


def test_empty_agent_config():
    cluster = Cluster(
        id=1,
        name='meeseeks',
        slug='meeseeks',
        variables={},
        prober_configs=[]
    )

    with patch.object(AgentConfigS3Downloader, "_get_cluster", return_value=(cluster, False)):
        config = S3AgentConfigLoader().load()

    assert len(config.probers) == 0


def test_agent_not_effective_configs_loader():
    cluster = Cluster(
        id=1,
        name='meeseeks',
        slug='meeseeks',
        variables={},
        prober_configs=[
            ProberConfig(
                prober_id=1,
                hosts_re='localhost',
                is_prober_enabled=True,
                interval_seconds=10,
                timeout_seconds=10,
                s3_logs_policy=UploadProberLogPolicy.ALL
            )
        ]
    )

    prober = Prober(
        id=1,
        name="External DNS request",
        slug="dns-external",
        runner=BashProberRunner(
            command="dig yandex.ru",
        ),
        files=[]
    )

    with patch.object(AgentConfigS3Downloader, "_get_cluster", return_value=(cluster, False)):
        with patch.object(AgentConfigS3Downloader, "_get_prober", return_value=(prober, False)):
            with patch.object(AgentConfigS3Downloader, "_is_matched_with_hosts_re", return_value=False):
                config = S3AgentConfigLoader().load()

    assert len(config.probers) == 0


def test_prober_parsing():
    prober_json = json.dumps(
        {
            "id": 1,
            "name": "External DNS request",
            "slug": "dns-external",
            "runner": {
                "type": "BASH",
                "command": "dig yandex.ru",
            },
            "files": [{
                "id": 1,
                "is_executable": True,
                "relative_file_path": "curl.sh",
                "md5_hexdigest": "a2e14daf695d4b76eb60ed9c91f450fd",
            }]
        }
    )
    prober = Prober.parse_raw(prober_json)

    assert prober.id == 1
    assert prober.name == "External DNS request"
    assert prober.slug == "dns-external"
    assert isinstance(prober.runner, BashProberRunner)
    assert prober.runner.command == "dig yandex.ru"
    assert len(prober.files) == 1
    assert prober.files[0].id == 1
    assert prober.files[0].is_executable
    assert prober.files[0].relative_file_path == "curl.sh"
    assert prober.files[0].md5_hexdigest == "a2e14daf695d4b76eb60ed9c91f450fd"


def test_agent_config_variables_expand_from_cluster_variables():
    cluster_slug = "preprod"
    cluster_variables = {
        "empty_string": "",
        "str": "test string",
        "int": 128,
        "float": 36.6,
        "list": ["1", "2", "3"],
        "dict": {
            "debug": "sub key value"
        },
        "level": "debug",
        "debug_format": "some value",
    }
    prober_config_variables = {
        "cluster_id": "${cluster.id}",
        "cluster_name": "${cluster.name}",
        "cluster_slug": "${cluster.slug}",
        "cluster_var_empty_string": "${cluster.variables.empty_string}",
        "cluster_var_str": "${cluster.variables.str}",
        "cluster_var_int": "${cluster.variables.int}",
        "cluster_var_float": "${cluster.variables.float}",
        "cluster_var_list": "${cluster.variables.list}",
        "cluster_var_dict": "${cluster.variables.dict}",
        "cluster_var_partial_str": "part:${cluster.variables.str}",
        "cluster_var_partial_int": "part:${cluster.variables.int}",
        "cluster_var_partial_float": "part1:${cluster.variables.float}",
        "custom_shell_var": "${{HOST}}",
        "nested_variable_1": "${cluster.variables.${cluster.variables.level}_format}",
        "nested_variable_2": "${cluster.variables.dict.${cluster.variables.level}}",
        "timeout": "${variables.timeout_${cluster.slug}}",
        "timeout_preprod": 10,
    }

    prober = Prober(id=1, name="Test Prober", slug="test-prober", runner=BashProberRunner(command="echo OK"), files=[])
    cluster = Cluster(
        id=1, name='Test Cluster', slug=cluster_slug, variables=cluster_variables,
        prober_configs=[
            ProberConfig(
                prober_id=1,
                is_prober_enabled=True,
                interval_seconds=10,
                timeout_seconds=10,
                variables=prober_config_variables,
                s3_logs_policy=UploadProberLogPolicy.FAIL
            ),
        ]
    )
    expected_prober_config_variables = {
        "cluster_id": cluster.id,
        "cluster_name": cluster.name,
        "cluster_slug": cluster.slug,
        "cluster_var_empty_string": cluster.variables["empty_string"],
        "cluster_var_str": cluster.variables["str"],
        "cluster_var_int": cluster.variables["int"],
        "cluster_var_float": cluster.variables["float"],
        "cluster_var_list": cluster.variables["list"],
        "cluster_var_dict": cluster.variables["dict"],
        "cluster_var_partial_str": "part:" + cluster.variables["str"],
        "cluster_var_partial_int": "part:" + str(cluster.variables["int"]),
        "cluster_var_partial_float": "part1:" + str(cluster.variables["float"]),
        "custom_shell_var": "${HOST}",
        "nested_variable_1": cluster.variables["debug_format"],
        "nested_variable_2": cluster.variables["dict"]["debug"],
        "timeout": 10,
        "timeout_preprod": 10,
    }
    get_cluster_ret_val = [cluster, False]

    with patch.object(AgentConfigS3Downloader, "_get_cluster", return_value=get_cluster_ret_val) as get_cluster_mock, \
            patch.object(AgentConfigS3Downloader, "_get_prober", return_value=(prober, False)) as get_prober_mock, \
            patch.object(AgentConfigS3Downloader, "_is_matched_with_hosts_re", return_value=True):
        config_loader = S3AgentConfigLoader()
        config = config_loader.load()
        assert len(config.probers) == 1
        get_cluster_mock.assert_called_once()
        get_prober_mock.assert_called_once()
        assert config.probers[0].config.variables == expected_prober_config_variables

        # mark that cluster returns from cache
        get_cluster_ret_val[1] = True
        config2 = config_loader.load()
        assert config2 == config

        # check that config returns from cache
        get_prober_mock.assert_called_once()


# TODO (nuraev): добавить переменные через config.variables.var_name чтобы проверить что они тоже доступны

@pytest.mark.parametrize(
    "prober_config_matrix,prober_config_variables,expected_prober_config_variables,expected_exception,expected_exceptions_string",
    [
        [
            {},
            {"a": "${variables.a}"},
            None,
            Exception,
            "the variable 'a' refer to itself",
        ],
        [
            {},
            {
                "a": "${variables.b}",
                "b": "${variables.a}",
            },
            None,
            Exception,
            "variables 'a, b' have a cyclic dependence",
        ],
        [
            {},
            {
                "a": "${variables.b}",
                "b": "${variables.c}",
                "c": "${variables.a}",
            },
            None,
            Exception,
            "variables 'a, b, c' have a cyclic dependence",
        ],
        [
            {},
            {
                "a": "${variables.b}",
                "b": "${variables.c}",
                "c": "${variables.d}",
            },
            None,
            Exception,
            "variable 'a' can not be substituted: variable 'variables.d' not found",
        ],
        [
            {},
            {
                "a": "${variables.}",
            },
            None,
            Exception,
            "empty key after position",
        ],
        [
            {},
            {
                "a": "${variables.c}",
                "b": "${variables.a}",
                "c": 10,
            },
            [{"a": 10, "b": 10, "c": 10}],
            None, None
        ],
        [
            {
                "host": ["yandex.ru", "google.com", "andgein.ru"],
                "ip_version": [4, 6]
            },
            {
                "host": "${matrix.host}",
                "url": "https://${ matrix.host }",
                "ip_version": "${ matrix.ip_version }",
            },
            [
                {"host": "yandex.ru", "url": "https://yandex.ru", "ip_version": 4},
                {"host": "yandex.ru", "url": "https://yandex.ru", "ip_version": 6},
                {"host": "google.com", "url": "https://google.com", "ip_version": 4},
                {"host": "google.com", "url": "https://google.com", "ip_version": 6},
                {"host": "andgein.ru", "url": "https://andgein.ru", "ip_version": 4},
                {"host": "andgein.ru", "url": "https://andgein.ru", "ip_version": 6},
            ],
            None, None
        ],
        [
            {
                "host": ["yandex.ru", "google.com", "andgein.ru"],
            },
            {
                "ip_version": "${matrix.ip_version}",
            },
            None,
            Exception,
            "variable 'ip_version' can not be substituted: variable 'matrix.ip_version' not found",
        ]
    ],
    ids=[
        "var refer itself",
        "2 vars refer to each other",
        "3 vars have cyclic dependency",
        "var can not be substituted",
        "empty key error",
        "var already resolved in recurse",
        "matrix usage",
        "matrix invalid usage",
    ],
)
def test_agent_config_variables_expand_from_cluster_variables_different_cases(
    prober_config_matrix: Dict[str, List[Any]],
    prober_config_variables: Dict[str, Any],
    expected_prober_config_variables: Optional[List[Dict[str, Any]]],
    expected_exception: Optional[Type[Exception]],
    expected_exceptions_string: Optional[str],
):
    prober_config_variables = OrderedDict(prober_config_variables)
    prober = Prober(id=1, name="Test Prober", slug="test-prober", runner=BashProberRunner(command="echo OK"), files=[])
    cluster = Cluster(
        id=1, name='Test Cluster', slug='test-cluster', variables={},
        prober_configs=[
            ProberConfig(
                prober_id=1,
                is_prober_enabled=True,
                interval_seconds=10,
                timeout_seconds=10,
                matrix_variables=prober_config_matrix,
                variables=prober_config_variables,
                s3_logs_policy=UploadProberLogPolicy.FAIL
            ),
        ]
    )
    get_cluster_ret_val = [cluster, False]

    with patch.object(AgentConfigS3Downloader, "_get_cluster", return_value=get_cluster_ret_val), \
            patch.object(AgentConfigS3Downloader, "_get_prober", return_value=(prober, False)), \
            patch.object(AgentConfigS3Downloader, "_is_matched_with_hosts_re", return_value=True):
        config_loader = S3AgentConfigLoader()

        if expected_exception is not None:
            with pytest.raises(expected_exception) as ex:
                config_loader.load()

            if expected_exceptions_string:
                assert expected_exceptions_string in str(ex)
        else:
            config = config_loader.load()
            assert expected_prober_config_variables == [prober.config.variables for prober in config.probers]


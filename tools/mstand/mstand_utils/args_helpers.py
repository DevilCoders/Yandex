# -*- coding: utf-8 -*-

import argparse  # noqa
import logging
import os.path
from typing import Optional

import mstand_utils.mstand_tables as mstand_tables
import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile

from mstand_enums.mstand_general_enums import YsonFormats, YtDownloadingFormats
from mstand_enums.mstand_offline_enums import OfflineCalcMode
from mstand_enums.mstand_online_enums import TableGroupEnum
from .mstand_def_values import OfflineDefaultValues


def add_input_pool(parser: argparse.ArgumentParser, required: bool = True, multiple: bool = False):
    return uargs.add_input(parser, help_message="read source pool from this file",
                           required=required, multiple=multiple)


def add_output_pool(parser, help_message="write pool to this file"):
    parser.add_argument(
        "--output-pool",
        help=help_message
    )


def add_output_tsv(parser, help_message="write tsv to this file"):
    parser.add_argument(
        "--output-tsv",
        help=help_message,
    )


def add_output_json(parser, required=False):
    parser.add_argument(
        "--output-json",
        help="write json to this file",
        required=required
    )


def add_output_wiki(parser):
    parser.add_argument(
        "--output-wiki",
        help="write wiki to this file",
    )


def add_output_html(parser):
    parser.add_argument(
        "--output-html",
        help="write html to this file",
    )


def add_output_wiki_vertical(parser):
    parser.add_argument(
        "--output-wiki-vertical",
        help="write wiki (vertical format) to this file",
    )


def add_output_html_vertical(parser):
    parser.add_argument(
        "--output-html-vertical",
        help="write HTML (vertical format) to this file",
    )


def add_yt(parser):
    """
    :type parser: argparse.ArgumentParser
    """
    add_yt_server(parser)
    add_yt_pool(parser)
    uargs.add_boolean_argument(
        parser,
        "filter-so",
        help_message="do not upload .so files (and hashlib module) to YT",
        default=True
    )
    parser.add_argument(
        "--ping-failed-mode",
        default=None,
        choices=("interrupt_main", "send_signal", "pass"),
        help="Ping failed mode (default: None)",
    )


def add_yt_server(parser):
    parser.add_argument(
        "-s",
        "--server",
        "--yt-server",
        default="hahn",
        help="YT server (default: hahn)",
    )


# in offline calculations, 'yt_server' conflicts with -s(--serpset-id)
def add_yt_cluster(parser, required=False):
    parser.add_argument(
        "--yt-cluster",
        "--cluster",
        help="YT cluster (default: autodetect)",
        required=required
    )


def add_yt_pool(parser):
    parser.add_argument(
        "--yt-pool",
        default=None,
        help="YT Pool (default: None)",
    )


def add_modules(parser, default_class=None, default_module=None, required=True):
    """
    :type parser: argparse.ArgumentParser
    :type default_class: str
    :type default_module: str
    :type required: bool
    """
    uargs.add_module_params(parser, default_class=default_class, default_module=default_module, required=required)
    uargs.add_boolean_argument(parser, "lamps-mode", help_message="calc built-in 'lamp' metrics", default=False)
    parser.add_argument("--plugin-dir", help="take plugin folder location")


def add_transactions(parser):
    """
    :type parser: argparse.ArgumentParser
    """
    parser.add_argument(
        "--transactions",
        type=int,
        help="limit MR transaction count",
    )


def add_dates(parser, required=True):
    """
    :type parser: argparse.ArgumentParser
    :type required: bool
    """
    parser.add_argument(
        "--date-from",
        required=required,
        help="experiment start date (YYYYMMDD)",
    )
    parser.add_argument(
        "--date-to",
        required=required,
        help="experiment end date (YYYYMMDD)",
    )


def add_save_cache(parser: argparse.ArgumentParser,
                   help_message: str = "write compressed cache to this file (.tgz format)", required: bool = False):
    parser.add_argument(
        "--save-cache",
        help=help_message,
        required=required
    )


def add_load_cache(parser: argparse.ArgumentParser,
                   help_message: str = "unpack cache data from this file (.tgz format)", required: bool = False):
    parser.add_argument(
        "--load-cache",
        help=help_message,
        required=required
    )


def add_storage_params(parser):
    parser.add_argument(
        "--root-cache-dir",
        default=".",
        help="root directory for cache",
    )
    parser.add_argument(
        "--cache-subdir",
        default="work_dir",
        help="cache subdirectory (will be created in root-cache-dir)",
    )
    add_use_cache(parser)


def add_offline_metric_calc_options(parser, def_use_internal_output, def_use_external_output, def_gzip_mc_output):
    uargs.add_output(parser, help_message="write pool with metric results to this file")
    add_metric_props(parser)
    add_plugin_params(parser)
    add_save_to_tar(parser, help_message="save metric results with pool to .tgz archive")
    add_yt_auth_token_file(parser)
    uargs.add_boolean_argument(parser, "load-urls", default=True, help_message="load URL data")
    uargs.add_boolean_argument(parser, "collect-scale-stats", default=False, help_message="collect scale stats")
    uargs.add_boolean_argument(parser, "skip-failed-serps", default=False, help_message="skip failed serps")
    uargs.add_boolean_argument(parser, "numeric-values-only", default=False, help_message="Allow numeric values only")
    parser.add_argument("--calc-mode", choices=OfflineCalcMode.ALL, default=OfflineCalcMode.LOCAL,
                        help="Metrics calculation mode (auto/local/yt)")

    # former mc_calc_options

    add_mc_output(parser, def_gzip_mc_output=def_gzip_mc_output)
    add_mc_alias_prefixes(parser)
    add_skip_metric_errors(parser)
    add_use_internal_output(parser, def_use_internal_output=def_use_internal_output)
    add_use_external_output(parser, def_use_external_output=def_use_external_output)
    add_mc_yt_root_path(parser)
    add_mc_yt_output_ttl(parser)
    add_yt_cluster(parser)
    add_yt_pool(parser)


def add_use_external_output(parser, def_use_external_output):
    uargs.add_boolean_argument(parser, "use-external-output", default=def_use_external_output, help_message="Use external (Metrics) output")


def add_metric_props(parser):
    parser.add_argument(
        "--set-coloring",
        help="override metric coloring",
    )
    parser.add_argument(
        "--set-alias",
        help="override metric name",
    )


def add_postprocess_props(parser):
    parser.add_argument(
        "--set-alias",
        help="override postprocessor name",
    )


def add_toloka_filtering_props(parser):
    parser.add_argument(
        "--set-alias",
        help="override toloka filtering name",
    )


def add_plugin_params(parser):
    add_modules(parser, required=False)

    parser.add_argument(
        "-b",
        "--batch",
        "--metric-batch",
        help="Take plugin list and parameters from this file",
    )


def add_use_cache(parser):
    uargs.add_boolean_argument(parser, "use-cache", "Use cache for serpsets and metric results", default=True)


def add_serp_attributes(parser):
    parser.add_argument(
        "--sitelink-attrs",
        "--sitelink-requirements",
        "--sitelink-reqs",
        help="Comma-separated SITELINK-level requirements",
    )
    parser.add_argument(
        "--component-attrs",
        "--component-requirements",
        "--component-reqs",
        help="Comma-separated COMPONENT-level requirements (e.g. dimension.IMAGE_DIMENSION)",
    )

    parser.add_argument(
        "--serp-attrs",
        "--serp-requirements",
        "--serp-reqs",
        help="Comma-separated SERP-level requirements",
    )

    parser.add_argument(
        "--judgements",
        help="Comma-separated judgements (alias for COMPONENT.judgement.<judgement>)",
    )


def add_serpset_id(parser, required=True):
    parser.add_argument(
        "-s",
        "--serpset-id",
        nargs="+",
        required=required,
        help="Serpset IDs",
    )


def add_serpset_id_filter(parser, required=False):
    parser.add_argument(
        "--serpset-id-filter",
        nargs="+",
        required=required,
        help="Take only this serpset ID (for debug purpose)",
    )


def add_nirvana_workflow_uids(parser, required=True):
    parser.add_argument(
        "--nirvana-workflow-uids",
        nargs="+",
        required=required,
        help="list of nirvana workflow (graph) uids",
    )


def add_mc_yt_root_path(parser):
    parser.add_argument(
        "--mc-yt-root-path",
        default="//home/mstand-offline/prod",
        help="YT root path for offline metric tables",
    )


def add_mc_yt_output_ttl(parser):
    parser.add_argument(
        "--mc-yt-output-ttl",
        default="5d",
        help="Offline metrics YT output TTL (seconds or '24h', '1d', '12h30m', etc)",
    )


def add_mc_alias_prefixes(parser):
    parser.add_argument("--mc-alias-prefix", help="metric alias prefix", default="metric.")
    parser.add_argument("--mc-error-alias-prefix", help="metric error alias prefix", default="metricError.")


def add_mc_output(parser, def_gzip_mc_output):
    parser.add_argument("--mc-output", help="output .json or .tgz file with metric results for Metrics")
    parser.add_argument("--mc-output-mr-table", help="output MR table with metric results for Metrics")
    parser.add_argument("--mc-broken-metrics", help="broken metric modules/classes info")
    uargs.add_boolean_argument(parser, "gzip-mc-output",
                               default=def_gzip_mc_output, help_message="gzip Metrics output")


def add_mc_serpsets_tar(parser, required=False):
    parser.add_argument("--mc-serpsets-tar", help="serpsets tar in mc format", required=required)


def add_skip_metric_errors(parser):
    uargs.add_boolean_argument(parser, "skip-metric-errors",
                               default=False, help_message="skip metric errors (Metrics mode)")


def add_use_internal_output(parser, def_use_internal_output):
    uargs.add_boolean_argument(parser, name="use-internal-output", inverted_name="no-use-internal-output",
                               default=def_use_internal_output, help_message="use 'native' mstand output format")


def add_yt_auth_token_file(parser: argparse.ArgumentParser,
                           default_token_path: str = os.path.expanduser("~/.yt/token")) -> None:
    parser.add_argument(
        "--yt-auth-token-file",
        help="YT OAuth token file",
        default=default_token_path
    )


def add_serp_fetch_params(parser, use_external_convertor_default):
    # metrics interaction params + serpset fetch options

    uargs.add_boolean_argument(parser, "skip-broken-serpsets", "Skip broken serpsets", default=False)

    add_serp_fetch_threads(parser)
    add_serp_convert_threads(parser)
    add_serp_unpack_threads(parser)

    parser.add_argument(
        "--serp-set-filter",
        "--ssf",
        "--mc-serp-set-filter",
        "--serpset-filter",
        help="Metrics serp-set-filter (aka component filter) parameter, see https://wiki.yandex-team.ru/metrics/API/serp/filters/#komponentnyefiltry"
    )

    parser.add_argument(
        "--pre-filter",
        "--mc-pre-filter",
        "--query-filter",
        help="Metrics pre-filter (per-query filter) parameter, see https://wiki.yandex-team.ru/metrics/API/serp/filters/#serpovyefiltry"
    )

    parser.add_argument(
        "--mc-aspect",
        "--aspect",
        help="aspect (default, etc.)",
    )

    parser.add_argument(
        "--mc-retry-timeout",
        type=float,
        default=30,
        help="retry timeout before first retry (30 seconds by default with exp.backoff)",
    )
    parser.add_argument(
        "--mc-retry-count",
        type=int,
        default=5,
        help="retry count before failing (5 by default)",
    )

    parser.add_argument(
        "--mc-server",
        default="metrics-calculation.metrics.yandex-team.ru",
        help="Metrics server host name (use production if not set)",
    )

    parser.add_argument(
        "--mc-auth-token-file",
        help="Metrics OAuth token file",
    )

    uargs.add_boolean_argument(parser, "use-external-convertor",
                               help_message="use json-to-jsonlines external convertor (faster)",
                               default=use_external_convertor_default)


def add_serp_parse_params(parser):
    uargs.add_boolean_argument(parser, "allow-no-position",
                               default=True, help_message="Allow missing position field in serps")

    uargs.add_boolean_argument(parser, "allow-broken-components",
                               default=False, help_message="Allow broken components mode")
    uargs.add_boolean_argument(parser, "remove-raw-serpsets",
                               default=False, help_message="Remove raw serpsets after parsing")


def add_buckets(parser):
    # noinspection PyTypeChecker
    uargs.add_boolean_argument(parser, "buckets", "use ABT buckets", default=None)


def add_check_versions(parser):
    uargs.add_boolean_argument(parser, "check-versions", "check if all tables have the latest squeeze versions")


def add_common_session_metric_args(parser):
    uargs.add_verbosity(parser)
    add_min_versions(parser)
    add_plugin_params(parser)
    add_metric_props(parser)
    add_list_of_online_services(parser, possible=None)
    add_history(parser)
    add_buckets(parser)
    add_check_versions(parser)
    add_experiment_batch_args(parser)


def add_experiment_batch_args(parser):
    parser.add_argument(
        "--experiment-batch-min",
        type=int,
        help="how many experiments to handle in one operation (default: 3)",
        default=3,
    )
    parser.add_argument(
        "--experiment-batch-max",
        type=int,
        help="how many experiments to handle in one operation (default: 10)",
        default=10,
    )


def add_common_session_metric_local_args(parser):
    add_common_session_metric_args(parser)
    add_squeeze_path(parser, "./squeeze")


def add_common_session_metric_yt_args(parser):
    add_common_session_metric_args(parser)
    add_yt(parser)
    add_squeeze_path(parser, "//home/mstand/squeeze")
    parser.add_argument(
        "--copy-results-to-yt-dir",
        help="path to YT directory",
    )
    uargs.add_boolean_argument(
        parser,
        "--yt-results-only",
        help_message="do not download metric results from YT (use only with --copy-results-to-yt-dir)",
        default=False,
    )
    parser.add_argument(
        "--yt-results-ttl",
        help="TTL in days for tables with results (use only with --copy-results-to-yt-dir)",
        type=int,
    )
    uargs.add_boolean_argument(
        parser,
        "--split-metric-results",
        help_message="metrics in yt tables are split into rows of user_id, metric_id, index, value",
        default=False,
    )
    uargs.add_boolean_argument(
        parser,
        "--split-values-and-schematize",
        help_message="metrics in yt tables with schema are split into rows of user_id, metric_id, index and other columns",
        default=False,
    )


def add_squeeze_path(parser: argparse.ArgumentParser, default_path: str = mstand_tables.DEFAULT_SQUEEZE_PATH) -> None:
    parser.add_argument(
        "--squeeze-path",
        default=default_path,
        help="path to squeeze (default: {})".format(default_path),
    )


def add_sessions_path(parser, default_path):
    parser.add_argument(
        "--sessions-path",
        default=default_path,
        help="path to user_sessions (default: {})".format(default_path),
    )


def add_yuids_path(parser, default_path, default_market_path):
    parser.add_argument(
        "--yuids-path",
        default=default_path,
        help="path to yuid_testids directory (default: {})".format(default_path),
    )
    parser.add_argument(
        "--yuids-market-path",
        default=default_market_path,
        help="path to market yuid_testids directory (default: {})".format(default_market_path),
    )


def add_zen_path(parser, default_path):
    parser.add_argument(
        "--zen-path",
        default=default_path,
        help="path to zen logs table (default: {})".format(default_path),
    )


def add_zen_sessions_path(parser, default_path):
    parser.add_argument(
        "--zen-sessions-path",
        default=default_path,
        help="path to zen sessions table (default: {})".format(default_path),
    )


def add_no_zen_sessions_flag(parser):
    uargs.add_boolean_argument(
        parser,
        "--no-zen-sessions",
        help_message="do not use zen sessions",
        default=False,
    )


def add_all_users_flag(parser):
    uargs.add_boolean_argument(
        parser,
        '--all-users',
        help_message='process all users (ignore testids)',
    )


def add_use_filters_flag(parser):
    uargs.add_boolean_argument(
        parser,
        "--use-filters",
        help_message="use ABT filters",
        default=True,
    )


def add_list_of_services(parser, default=None, possible=None, help_message="use these services"):
    if default is not None and possible is not None:
        assert default in possible
    parser.add_argument(
        "--services",
        nargs="*",
        default=[default] if default else None,
        choices=possible,
        help=help_message,
    )


def add_list_of_testids(parser):
    parser.add_argument(
        "--testids",
        required=True,
        nargs="+",
        help="experiment test-id list",
    )


def add_one_testid(parser):
    parser.add_argument(
        "--testid",
        required=True,
        help="experiment test-id",
    )


def add_one_observation_id(parser, required=True):
    parser.add_argument(
        "--observation-id",
        required=required,
        help="observation id",
    )


def add_observation_ids(parser, help_message="observation IDs"):
    parser.add_argument(
        "observations",
        nargs="*",
        help=help_message,
    )


def add_save_to_file(parser, help_message="save results to file"):
    parser.add_argument(
        "--save-to-file",
        help=help_message,
    )


def add_save_to_dir(parser, help_message="save results to dir"):
    parser.add_argument(
        "--save-to-dir",
        help=help_message,
    )


def add_save_to_yt_dir(parser: argparse.ArgumentParser, help_message: str = "save results to yt dir") -> None:
    parser.add_argument(
        "--save-to-yt-dir",
        help=help_message,
    )


def add_save_to_tar(parser, help_message="save results to tar"):
    parser.add_argument(
        "--save-to-tar",
        help=help_message,
    )


def add_min_versions(parser):
    parser.add_argument(
        "--min-versions",
        help="minimal requirements for squeeze versions"
    )


def add_history(parser):
    parser.add_argument(
        "--history",
        type=int,
        help="get user actions for N day before experiment",
    )
    parser.add_argument(
        "--future",
        type=int,
        help="get user actions for N day after experiment",
    )


def add_min_pvalue(parser, default=0.001):
    parser.add_argument(
        "--min-pvalue",
        type=float,
        default=default,
        help="min_pvalue for S function (default: {})".format(default),
    )


def add_threshold(parser, default=0.01):
    parser.add_argument(
        "--threshold",
        type=float,
        default=default,
        help="mark as RED or GREEN only if pvalue < threshold (default: {})".format(default),
    )


def add_download_threads(parser, default=1, help_message="limit threads count for downloading tables"):
    """
    :type parser: argparse.ArgumentParser
    :type default: int
    :type help_message: str
    """
    parser.add_argument(
        "--download-threads",
        type=int,
        help="{} (default: {})".format(help_message, default),
        default=default,
    )


def add_sort_threads(parser, default=1, help_message="limit threads count for sorting tables"):
    """
    :type parser: argparse.ArgumentParser
    :type default: int
    :type help_message: str
    """
    parser.add_argument(
        "--sort-threads",
        type=int,
        help="{} (default: {})".format(help_message, default),
        default=default,
    )


def add_lower_reducer_key(parser, help_message="set lower_key at reducer"):
    parser.add_argument(
        "--lower-reducer-key",
        help=help_message,
        default=None,
    )


def add_upper_reducer_key(parser, help_message="set upper_key at reducer"):
    parser.add_argument(
        "--upper-reducer-key",
        help=help_message,
        default=None,
    )


def add_start_mapper_index(parser, help_message="set start_index at mapper"):
    parser.add_argument(
        "--start-mapper-index",
        type=int,
        help=help_message,
        default=None,
    )


def add_lower_filter_key(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--lower-filter-key",
        help="lower yuid key (included)",
        required=True,
    )


def add_upper_filter_key(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--upper-filter-key",
        help="upper yuid key (not included)",
        required=True,
    )


def add_end_mapper_index(parser, help_message="set end_index at mapper"):
    parser.add_argument(
        "--end-mapper-index",
        type=int,
        help=help_message,
        default=None,
    )


def add_list_of_online_services(parser, possible):
    add_list_of_services(parser, default="web-auto", possible=possible)


def add_yt_memory_limit(parser, default=512):
    parser.add_argument(
        "--yt-memory-limit",
        type=int,
        default=default,
        help="YT memory limit in MB (default: {}MB)".format(default),
    )


def add_yt_data_size_per_job(parser, default=300):
    parser.add_argument(
        "--yt-data-size-per-job",
        type=int,
        default=default,
        help="YT memory limit for map/reduce operation jobs in MB (default: {}MB)".format(default)
    )


def add_max_data_size_per_job(parser, default=200):
    parser.add_argument(
        "--max-data-size-per-job",
        type=int,
        default=default,
        help="Max YT memory limit for map/reduce operation jobs in GB (default: {}GB)".format(default)
    )


def add_yt_tentative_options(parser, default_enable=True):
    uargs.add_boolean_argument(
        parser,
        "yt-tentative-enable",
        inverted_name="yt-tentative-disable",
        help_message="enable tentative job",
        inverted_help_message="disable tentative job",
        default=default_enable,
    )
    parser.add_argument(
        "--yt-tentative-sample-count",
        type=int,
        default=10,
        help="max tentative job sample count (default: 10)",
    )
    parser.add_argument(
        "--yt-tentative-duration-ratio",
        type=float,
        default=10.0,
        help="max tentative job duration ratio (default: 10.0)",
    )


def add_yt_lock_options(parser):
    uargs.add_boolean_argument(
        parser,
        "yt-lock-enable",
        inverted_name="yt-lock-disable",
        help_message="enable squeeze lock",
        inverted_help_message="disable squeeze lock",
        default=True,
    )


def add_use_yt_cli_options(parser):
    uargs.add_boolean_argument(
        parser,
        "use-yt-cli",
        help_message="enable downloading with cli yt",
        default=False,
    )


def add_token(parser, name, default=None, help_message="OAuth token"):
    """
    :type parser: argparse.ArgumentParser
    :type name: str
    :type default: str | None
    :type help_message: str
    """
    if default is None:
        parser.add_argument(
            "--{}-token".format(name),
            help=help_message,
            required=True,
        )
    else:
        parser.add_argument(
            "--{}-token".format(name),
            help=help_message,
            default=default,
        )


def add_ttl(parser):
    """
    :type parser: argparse.ArgumentParser
    """
    parser.add_argument(
        "--ttl",
        help="job ttl, in minutes",
        type=int,
        required=True,
    )


def add_additional_ttl(parser):
    """
    :type parser: argparse.ArgumentParser
    """
    parser.add_argument(
        "--additional-ttl",
        help="job ttl for accompanying actions, in minutes",
        type=int,
        required=True,
    )


def add_source(parser, possible=None):
    """
    :type parser: argparse.ArgumentParser
    :type possible: list[str] | None
    """
    parser.add_argument(
        "--source",
        choices=possible,
        help="source for downloading",
    )


def add_service(parser, possible=None):
    """
    :type parser: argparse.ArgumentParser
    :type possible: list[str] | None
    """
    parser.add_argument(
        "--service",
        choices=possible,
        help="service for downloading",
    )


def add_mode(parser, choices, default, help_message="calculation mode"):
    """
    :type parser: argparse.ArgumentParser
    :type choices: tuple(str)
    :type default: str
    :type help_message: str
    """
    parser.add_argument(
        "--mode",
        choices=choices,
        help=help_message,
        default=default,
    )


def create_lamps_args(cli_args, lamps_batch=None):
    lamps_args = cli_args
    lamps_args.batch = lamps_batch
    lamps_args.class_name = None
    lamps_args.module_name = None
    lamps_args.user_kwargs = None
    lamps_args.set_alias = None
    lamps_args.lamps_mode = True
    lamps_args.set_coloring = "lamp"
    lamps_args.save_to_dir = None
    lamps_args.save_to_tar = None
    return lamps_args


def add_ignore_triggered_testids_filter(parser):
    uargs.add_boolean_argument(
        parser,
        "--ignore-triggered-testids-filter",
        help_message="ignore triggered_testids_filter"
    )


def get_token(path: str) -> Optional[str]:
    token_path = os.path.expanduser(path)
    if os.path.exists(token_path):
        logging.info("Got AB token from file %s", token_path)
        return ufile.read_token_from_file(token_path)
    else:
        logging.warning("Attention! To use mstand you should get AB token. See details https://wiki.yandex-team.ru/mstand/yt/#zapuskkubikovvnirvane")


def add_ab_token_file(parser: argparse.ArgumentParser, default_token_path: str = "~/.ab/token"):
    parser.add_argument(
        "--ab-token-file",
        help="AB API OAuth token file",
        default=default_token_path,
    )


def add_yql_token_file(parser: argparse.ArgumentParser, default_token_path: str = "~/.yql/token") -> None:
    parser.add_argument(
        "--yql-token-file",
        help="YQL API OAuth token file",
        type=os.path.expanduser,
        default=default_token_path,
    )


def add_serp_fetch_threads(parser, default=OfflineDefaultValues.FETCH_THREADS, help_message="limit serpset fetch threads"):
    """
    :type parser: argparse.ArgumentParser
    :type default: int
    :type help_message: str
    """
    parser.add_argument(
        "--fetch-threads",
        type=int,
        help="{} (default: {})".format(help_message, default),
        default=default,
    )


def add_serp_convert_threads(parser, default=OfflineDefaultValues.CONVERT_THREADS, help_message="limit serpset convert threads"):
    """
    :type parser: argparse.ArgumentParser
    :type default: int
    :type help_message: str
    """
    parser.add_argument(
        "--convert-threads",
        type=int,
        help="{} (default: {})".format(help_message, default),
        default=default,
    )


def add_serp_unpack_threads(parser, default=OfflineDefaultValues.UNPACK_THREADS, help_message="limit serpset unpack threads"):
    """
    :type parser: argparse.ArgumentParser
    :type default: int
    :type help_message: str
    """
    parser.add_argument(
        "--unpack-threads",
        type=int,
        help="{} (default: {})".format(help_message, default),
        default=default,
    )


def add_enable_nile(parser):
    uargs.add_boolean_argument(
        parser,
        name="--enable-nile",
        help_message="enable nile for services using libra",
        inverted_name="--disable-nile",
        inverted_help_message="disable nile at all",
        default=True,
    )


def add_yson_format(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--yson-format",
        choices=YsonFormats.ALL,
        help="yson format (not required if json is used)",
        default=YsonFormats.BINARY,
    )


def add_local_dumping_format(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--dumping-format",
        choices=YtDownloadingFormats.ALL,
        help="format used to dump tables locally",
        default=YtDownloadingFormats.YSON,
    )


def add_encode_utf8_flag(parser: argparse.ArgumentParser) -> None:
    uargs.add_boolean_argument(parser=parser,
                               name="--encode-utf8",
                               default=False,
                               help_message="use utf8 encoding (used if json format is chosen)")


def add_squeeze_bin(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--squeeze-bin-file",
        help="path to bin file to run squeeze (./squeeze_lib/bin/bin)",
        default="./squeeze_lib/bin/bin",
    )


def add_list_of_table_groups(parser):
    parser.add_argument(
        "--table-groups",
        nargs="*",
        default=[TableGroupEnum.CLEAN],
        choices=TableGroupEnum.ALL,
        help="table groups for user session tables",
    )

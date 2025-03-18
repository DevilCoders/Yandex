import argparse
import collections
import datetime
import itertools
import logging
import os
import time

from typing import Counter
from typing import List
from typing import Optional
from typing import Set
from typing import Union

import adminka.ab_cache
import adminka.ab_helpers
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.mstand_tables as mstand_tables
import yt.wrapper as yt
import yt.yson.yson_types as yson_types

from experiment_pool import pool_helpers as pool_helpers
from session_squeezer import services as squeezer_services
from session_squeezer.experiment_for_squeeze import ExperimentForSqueeze
from session_squeezer.squeeze_runner import SqueezeTask
from yaqutils import time_helpers as utime


YT_ALLOWED_NODE_TYPES = [
    "map_node",
    "table",
    "file"
]

YT_ATTRIBUTES = [
    "type",
    "access_time",
    "creation_time",
    "modification_time",
    "resource_usage",
    "count",
    "owner",
]

YT_DEFAULT_SQUEEZE_PATH = "//home/mstand/squeeze"


def get_timeout_limit_datetime(days_as_str: str) -> datetime.datetime:
    try:
        days = int(days_as_str)
        assert days >= 0
    except (ValueError, AssertionError):
        raise Exception("Timeout limit must be non-negative integer")
    return datetime.datetime.now(tz=datetime.timezone.utc) - datetime.timedelta(days=days)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="remove old unused tables from specific directory")
    parser.add_argument(
        "--path",
        required=True,
        help="YT path to clear (for example //home/mstand/squeeze)",
    )

    access_time_group = parser.add_mutually_exclusive_group(required=True)
    access_time_group.add_argument(
        "--access-time-limit",
        help="remove nodes with access_time less than this, "
             "supported formats: YYYY-MM-DD, YYYYMMDD, YYYY-MM-DD-HH-MM-SS etc.",
        type=utime.parse_human_readable_datetime_utc,
    )
    access_time_group.add_argument(
        "--access-timeout-limit",
        help="remove nodes with access_time earlier than this number of days ago",
        type=get_timeout_limit_datetime
    )

    modification_time_group = parser.add_mutually_exclusive_group()
    modification_time_group.add_argument(
        "--modification-time-limit",
        help="remove nodes with modification_time less than this (default=access-time-limit), "
             "supported formats: YYYY-MM-DD, YYYYMMDD, YYYY-MM-DD-HH-MM-SS etc.",
        type=utime.parse_human_readable_datetime_utc,
    )
    modification_time_group.add_argument(
        "--modification-timeout-limit",
        help="remove nodes with modification_time earlier than this number of days ago",
        type=get_timeout_limit_datetime,
    )

    creation_time_group = parser.add_mutually_exclusive_group()
    creation_time_group.add_argument(
        "--creation-time-limit",
        help="remove nodes with creation_time less than this (default=access-time-limit), "
             "supported formats: YYYY-MM-DD, YYYYMMDD, YYYY-MM-DD-HH-MM-SS etc.",
        type=utime.parse_human_readable_datetime_utc,
    )
    creation_time_group.add_argument(
        "--creation-timeout-limit",
        help="remove nodes with creation_time earlier than this number of days ago",
        type=get_timeout_limit_datetime,
    )

    parser.add_argument(
        "--invert",
        action="store_true",
        help="remove nodes with access_time later than access-time-limit",
    )
    parser.add_argument(
        "--remove-really",
        action="store_true",
        help="run remove for real (do not simulate)",
    )
    parser.add_argument(
        "--owners",
        nargs="*",
        help="white-list of owners to clear",
    )
    parser.add_argument(
        "--substring",
        help="remove nodes with substring in path",
    )
    parser.add_argument(
        "--without-substring",
        help="remove nodes without substring in path",
        nargs="*",
    )
    parser.add_argument(
        "--node-type",
        nargs="+",
        help="types of nodes to delete (map_node (dir), table, file), all types by default",
        choices=YT_ALLOWED_NODE_TYPES,
        default=YT_ALLOWED_NODE_TYPES
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="print extended logs including paths to remove",
    )

    mstand_uargs.add_yt_server(parser=parser)
    return parser.parse_args()


def get_tables_blacklist_by_pools(pools_path: Optional[str],
                                  services: List[str],
                                  pools: Optional[List[str]] = None,
                                  ab_token_file: str = "~/.ab/token",
                                  squeeze_path: Optional[str] = None) -> Set[str]:
    assert pools or pools_path
    assert services

    squeeze_path = squeeze_path or YT_DEFAULT_SQUEEZE_PATH
    pools = _get_and_validate_pool(pools=pools,
                                   pools_path=pools_path,
                                   services=services,
                                   ab_token_file=ab_token_file)
    destinations = _get_destination_tables(squeeze_path=squeeze_path, pools=pools)
    return destinations


def run_clearing(path_to_clear: str,
                 yt_client: yt.YtClient,
                 access_time_limit: datetime.datetime,
                 creation_time_limit: Optional[datetime.datetime] = None,
                 modification_time_limit: Optional[datetime.datetime] = None,
                 invert_access_time_limit: bool = False,
                 owners: Union[List[str], Set[str], None] = None,
                 substring: Optional[str] = None,
                 substring_blacklist: Optional[List[str]] = None,
                 tables_blacklist: Optional[Set[str]] = None,
                 remove_really: bool = False,
                 node_type: Optional[List[str]] = None,
                 verbose: bool = True) -> None:
    assert access_time_limit

    creation_time_limit = creation_time_limit or access_time_limit
    modification_time_limit = modification_time_limit or access_time_limit
    node_type = node_type or YT_ALLOWED_NODE_TYPES

    if isinstance(owners, (list, tuple)):
        owners = set(owners)

    def _object_filter(yt_object: yson_types.YsonType) -> bool:
        if owners and yt_object.attributes["owner"] not in owners:
            return False
        return (_check_access_time(utime.iso_8601_to_datetime(yt_object.attributes["access_time"]),
                                   access_time_limit,
                                   invert_access_time_limit) and
                _check_access_time(utime.iso_8601_to_datetime(yt_object.attributes["creation_time"]),
                                   creation_time_limit,
                                   invert_access_time_limit) and
                _check_access_time(utime.iso_8601_to_datetime(yt_object.attributes["modification_time"]),
                                   modification_time_limit,
                                   invert_access_time_limit))

    def _path_filter(path: str) -> bool:
        if substring and substring not in path:
            return False
        if substring_blacklist and any(substr in path for substr in substring_blacklist):
            return False
        return True

    _shape_search_log(path_to_clear=path_to_clear,
                      access_time_limit=access_time_limit,
                      creation_time_limit=creation_time_limit,
                      modification_time_limit=modification_time_limit,
                      invert_access_time_limit=invert_access_time_limit,
                      owners=owners,
                      substring=substring,
                      substring_blacklist=substring_blacklist,
                      node_type=node_type)
    nodes_to_remove = yt.search(
        root=path_to_clear,
        node_type=node_type,
        client=yt_client,
        attributes=YT_ATTRIBUTES,
        object_filter=_object_filter,
        path_filter=_path_filter,
    )

    dirs_to_remove: List[yson_types.YsonUnicode] = []
    files_to_remove: List[yson_types.YsonUnicode] = []
    tables_to_remove: List[yson_types.YsonUnicode] = []

    skipped_tables_count = 0
    for node in nodes_to_remove:
        if node.attributes["type"] == "map_node" and node.attributes["count"] == 0:
            dirs_to_remove.append(node)
        elif node.attributes["type"] == "file":
            files_to_remove.append(node)
        elif node.attributes["type"] == "table":
            if _table_can_be_removed(path=str(node), tables_blacklist=tables_blacklist):
                tables_to_remove.append(node)
            else:
                skipped_tables_count += 1

    if skipped_tables_count:
        logging.info("%d tables can't be removed", skipped_tables_count)

    if not dirs_to_remove and not files_to_remove and not tables_to_remove:
        logging.info("nothing to remove")
        return

    _log_stats(dirs_to_remove=dirs_to_remove,
               files_to_remove=files_to_remove,
               tables_to_remove=tables_to_remove,
               verbose=verbose)
    if not remove_really:
        logging.info("removing for real is enabled: add --remove-really to remove for real")
    else:
        logging.info("running removing for real\nCountdown:")
        for i in range(5, 0, -1):
            logging.info("%d", i)
            time.sleep(1)

        batch_client = yt.create_batch_client(client=yt_client, raise_errors=True)
        for node in itertools.chain(dirs_to_remove, files_to_remove, tables_to_remove):
            batch_client.remove(path=str(node))

        try:
            batch_client.commit_batch()
        except yt.YtBatchRequestFailedError as err:
            error_messages: str = "\n\t".join(inner_error["message"] for inner_error in err.inner_errors)
            logging.warning("removing has completed with %d errors:\n\t%s", len(err.inner_errors), error_messages)
        else:
            logging.info("removing has successfully completed")


def _convert_bytes_to_tb(size: Union[int, float]) -> float:
    return size / (1024 * yt.common.GB)


def _table_can_be_removed(path: str,
                          tables_blacklist: Optional[Set[str]] = None) -> bool:
    if tables_blacklist and path in tables_blacklist:
        return False

    mstand_testids = "//home/mstand/squeeze/testids/"
    if path.startswith(mstand_testids):
        parts = path[len(mstand_testids):].split("/")
        if len(parts) == 3:
            date = parts[-1]
            # see INFRAINCIDENTS-178
            if "20160916" <= date <= "20161026":
                return False

    mstand_history = "//home/mstand/squeeze/history/"
    if path.startswith(mstand_history):
        parts = path[len(mstand_history):].split("/")
        if len(parts) == 4:
            date = parts[-1]
            # see INFRAINCIDENTS-178
            if "20160916" <= date <= "20161026":
                return False

    if path.startswith("//home/fivecg/snipoffline/hitman/monitor/"):
        # keep snipoffline logs OFFLINESUP-159
        return False

    return True


def _shape_search_log(path_to_clear: str,
                      access_time_limit: datetime.datetime,
                      creation_time_limit: datetime.datetime,
                      modification_time_limit: datetime.datetime,
                      invert_access_time_limit: bool,
                      owners: Union[List[str], Set[str]],
                      substring: str,
                      substring_blacklist: List[str],
                      node_type: List[str]) -> None:
    comparison_sign: str = ">" if invert_access_time_limit else "<"
    searching_params: List[str] = [
        "access time {} {}".format(comparison_sign, access_time_limit.strftime("%Y-%m-%d")),
        "modification time {} {}".format(comparison_sign, modification_time_limit.strftime("%Y-%m-%d")),
        "creation_time {} {}".format(comparison_sign, creation_time_limit.strftime("%Y-%m-%d")),
    ]

    if owners:
        searching_params.append("Owners: {}".format(", ".join(owners)))
    if substring:
        searching_params.append("Containing {} in the path".format(substring))
    if substring_blacklist:
        substring_blacklist_str: str = " ".join(substring_blacklist)
        searching_params.append(
            "Ignoring paths containing following substrings: {}".format(substring_blacklist_str)
        )

    node_types: str = "s, ".join(node_type) + "s"
    params_str: str = "\n\t".join(searching_params)
    log_message: str = "Searching for {} in {} with following parameters:\n\t{}".format(node_types,
                                                                                        path_to_clear,
                                                                                        params_str)

    logging.info(log_message)


def _log_stats(dirs_to_remove: List[yson_types.YsonUnicode],
               files_to_remove: List[yson_types.YsonUnicode],
               tables_to_remove: List[yson_types.YsonUnicode],
               verbose: bool) -> None:
    size = 0
    chunks = 0
    nodes = 0

    owners_stat: Counter[str] = collections.Counter()

    for node in itertools.chain(dirs_to_remove, files_to_remove, tables_to_remove):
        size += node.attributes["resource_usage"]["disk_space"]
        chunks += node.attributes["resource_usage"]["chunk_count"]
        nodes += node.attributes["resource_usage"]["node_count"]
        owners_stat[node.attributes["owner"]] += 1

    if verbose:
        if tables_to_remove:
            logging.info("%d tables will be removed:\n\t%s", len(tables_to_remove), "\n\t".join(tables_to_remove))

        if dirs_to_remove:
            logging.info("%d dirs will be removed:\n\t%s", len(dirs_to_remove), "\n\t".join(dirs_to_remove))

        if files_to_remove:
            logging.info("%d files will be removed:\n\t%s", len(files_to_remove), "\n\t".join(files_to_remove))

    logging.info(
        "will free:\n\t%.3f TB disk space\n\t%d chunks\n\t%d nodes (%d tables, %d dirs, %d files)",
        _convert_bytes_to_tb(size),
        chunks,
        nodes,
        len(tables_to_remove),
        len(dirs_to_remove),
        len(files_to_remove),
    )

    if owners_stat:
        max_owner_len = max(len(owner) for owner in owners_stat)
        template = "{{:{}s}}: {{}}".format(max_owner_len + 1)
        logging.info(
            "owners' top:\n\t%s",
            "\n\t".join(template.format(owner, count) for owner, count in owners_stat.most_common()),
        )


def _check_access_time(access_time: datetime.datetime,
                       access_time_limit: datetime.datetime,
                       invert_access_time_limit: bool) -> bool:
    if invert_access_time_limit:
        return access_time > access_time_limit
    else:
        return access_time < access_time_limit


def _get_and_validate_pool(pools: Optional[List[str]],
                           pools_path: Optional[str],
                           services: List[str],
                           ab_token_file: str) -> List[pool_helpers.Pool]:
    validated_pools: List[pool_helpers.Pool] = []
    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(ab_token_file))
    if pools:
        pool_paths = pools
        logging.info("Got %d pools from command line arguments:\n\t%s", len(pool_paths), "\n\t".join(pool_paths))
    else:
        pool_paths = [
            os.path.join(path, filename)
            for (path, _, filenames) in os.walk(pools_path)
            for filename in filenames
            if filename.endswith(".json")
        ]
        logging.info("Got %d pools from %s:\n\t%s", len(pool_paths), pools_path, "\n\t".join(pool_paths))

    for pool_path in pool_paths:
        logging.info("Start pool processing: %s", pool_path)
        pool = pool_helpers.load_pool(pool_path)
        adminka.ab_helpers.validate_and_enrich(
            pool=pool,
            session=session,
            add_filters=True,
            services=services,
            ignore_triggered_testids_filter=True,
            allow_bad_filters=True,
        )
        validated_pools.append(pool)
    return validated_pools


def _get_destination_tables(squeeze_path: str, pools: List[pool_helpers.Pool]) -> Set[str]:
    destinations: Set[str] = set()
    for pool in pools:
        squeeze_tasks = SqueezeTask.prepare_squeeze_tasks(
            experiments=ExperimentForSqueeze.from_pool(pool),
            history=0,
            future=0,
            cache_checker=None,
            enable_binary=True,
        )

        for task in squeeze_tasks:
            for exp in task.experiments:
                filter_hash = (
                    exp.filters.filter_hash
                    if squeezer_services.has_filter_support(exp.service)
                    else None
                )
                table_dir = mstand_tables.mstand_experiment_dir(
                    squeeze_path=squeeze_path,
                    service=exp.service,
                    testid=exp.testid,
                    dates=exp.dates,
                    filter_hash=filter_hash,
                )
                destinations.add(os.path.join(table_dir, utime.format_date(task.day)))

    return destinations

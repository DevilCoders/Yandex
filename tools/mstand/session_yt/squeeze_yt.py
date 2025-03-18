import contextlib
import datetime
import json
import logging
import os
import sys
import time
import uuid
import functools

from typing import Any
from typing import Callable
from typing import Dict
from typing import Generator
from typing import List
from typing import Optional
from typing import Set
from typing import Tuple
from typing import Union

from yaqtypes import JsonDict

from yql.config import config

config.no_color = True
config.is_tty = True

import yt.wrapper as yt
import yt.wrapper.client as yt_client
import yt.wrapper.common as yt_common
import yt.yson as yt_yson

import mstand_utils.log_helpers as mstand_ulog
import mstand_utils.mstand_tables as mstand_tables
import mstand_utils.yt_helpers as mstand_uyt
import pytlib.raw_yt_operations as raw_yt_ops
import pytlib.yt_io_helpers as yt_io
import session_squeezer.services as squeezer_services
import session_yt.versions_yt as versions_yt
import yaqutils.json_helpers as ujson
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirvana
import yaqutils.six_helpers as usix
import yaqutils.time_helpers as utime

from mstand_enums.mstand_general_enums import SqueezeWayEnum
from mstand_enums.mstand_online_enums import ScriptEnum, ServiceSourceEnum, ServiceEnum, RectypeEnum, TableTypeEnum
from mstand_utils.mstand_paths_utils import SqueezePaths
from mstand_structs.squeeze_versions import SqueezeVersions
from mstand_utils.yt_options_struct import YtJobOptions, TableBounds
from session_squeezer.experiment_for_squeeze import ExperimentForSqueeze
from session_squeezer.squeeze_runner import SqueezeTask
from session_squeezer.squeezer import UserSessionsSqueezer
from session_squeezer.squeezer_app_metrics_toloka import get_toloka_user_id
from session_squeezer.squeezer_ether import ether_mapper
from session_squeezer.squeezer_toloka import ActionsSqueezerToloka
from session_squeezer.squeezer_ya_video import ya_video_mapper
from yaqlibenums import YtOperationTypeEnum


class YTBackendLocalFiles:
    def __init__(self, use_filters: bool, services: List[str], source_dir: Optional[str] = None) -> None:
        # TODO: handle web-auto
        # services = squeezer_services.convert_aliases(services)
        # squeezers = squeezer_services.create_service_squeezers(services)
        # if not any(sq.USE_LIBRA for sq in squeezers.itervalues()):
        if all(s == ServiceEnum.TOLOKA for s in services):
            files = []
        else:
            files = [
                "blockstat.dict",
                "mousetrack_decoder.py",
                "browser.xml",
                "libra.so",
            ]

            if sys.version_info.minor == 8:
                files.append("py3.8/bindings.so")
            else:
                files.append("bindings.so")

            if use_filters:
                files.extend([
                    "geodata4.bin"
                ])

        if source_dir:
            files = [os.path.join(source_dir, file) for file in files]

        for filename in files:
            if not ufile.is_not_empty_file(filename):
                raise Exception("Required file {} not found", filename)

        self.files = files
        self.yt_files = None


class YTBackendStatboxFiles:
    def __init__(self, use_filters: bool, services: List[str]) -> None:
        # TODO: handle web-auto
        # services = squeezer_services.ServiceEnum.convert_aliases(services)
        # squeezers = squeezer_services.ServiceEnum.create_service_squeezers(services)
        # if not any(sq.USE_LIBRA for sq in squeezers.itervalues()):
        if all(s == ServiceEnum.TOLOKA for s in services):
            self.files = []
            self.yt_files = []
        else:
            self.files = []
            self.yt_files = [
                "//home/mstand/resources/blockstat.dict",
                "//home/mstand/resources/browser.xml",
            ]
            if use_filters:
                self.yt_files.append("//home/mstand/resources/geodata4.bin")

        if not usix.is_arcadia():
            self.files.append("mousetrack_decoder.py")
            self.yt_files.append("//home/mstand/resources/libra.so")
            if sys.version_info.minor == 8:
                self.yt_files.append("//home/mstand/resources/py3.8/bindings.so")
            else:
                self.yt_files.append("//home/mstand/resources/bindings.so")

        for filename in self.yt_files:
            if not yt.exists(filename):
                raise Exception("Required file {} not found", filename)

        for filename in self.files:
            if not ufile.is_not_empty_file(filename):
                raise Exception("Required file {} not found", filename)


class SqueezeYTReducer:
    def __init__(self, squeezer, exp_table_indexes, key, table_types):
        self.squeezer = squeezer
        self.exp_table_indexes = exp_table_indexes
        self.key = key
        self.table_indexes = table_types

    def __call__(self, keys, rows):
        key = keys[self.key]
        for exp, action in self.squeezer.squeeze_session(key, rows, self.table_indexes):
            index = self.exp_table_indexes[exp]
            action["@table_index"] = index
            yield action


class SqueezeNileReducer:
    def __init__(self, squeezer, exp_table_indexes):
        self.squeezer = squeezer
        self.exp_table_indexes = exp_table_indexes

    def __call__(self, sessions, *outputs):
        from nile.api.v1 import Record

        for key, container in sessions:
            for exp, action in self.squeezer.squeeze_nile_session(key, container):
                index = self.exp_table_indexes[exp]
                outputs[index](Record.from_dict(action))


class SqueezeYTTolokaMapper:
    def __init__(self, squeezer, exp_table_indexes, toloka_table):
        self.squeezer = squeezer
        self.exp_table_indexes = exp_table_indexes
        self.toloka_table = toloka_table

    def __call__(self, row):
        yuid = row["worker_id"]
        for exp, action in self.squeezer.squeeze_toloka(yuid, row, toloka_table=self.toloka_table):
            index = self.exp_table_indexes[exp]
            action["@table_index"] = index
            yield action


@yt.with_context
class SqueezeYTTolokaJoinMapper:
    def __init__(self, squeezer, exp_table_indexes, toloka_table):
        self.squeezer = squeezer
        self.exp_table_indexes = exp_table_indexes
        self.toloka_table = toloka_table

    def __call__(self, row, context):
        # 0 означает, что будем подклеивать данные к событиям этой таблицы
        if context.table_index == 0:
            yuid = row["worker_id"]
            for exp, action in self.squeezer.squeeze_toloka(yuid, row, toloka_table=self.toloka_table):
                index = self.exp_table_indexes[exp]
                action["_table_index"] = index
                action["worker_id"] = row["worker_id"]
                yield action
        # иначе мы в таблице, из которой берём данные для подклейки
        else:
            row["_table_index"] = -1
            yield row


@yt.with_context
class SqueezeYTTolokaJoinReducer:
    def __call__(self, key, _rows, context):
        registration_source = None
        registration_platform = None
        registration_date = None
        # индекс таблицы, из которой нужно взять данные для подклейки
        worker_attributes_table_index = -1
        for row in _rows:
            if row["_table_index"] == worker_attributes_table_index:
                registration_source = row.get('source')
                registration_platform = row.get('platform')
                registration_date = row.get('date')
            else:
                if "worker_attributes_computed" not in row or row["worker_attributes_computed"] is None:
                    row["worker_attributes_computed"] = {}
                row["worker_attributes_computed"]["registration_source"] = registration_source
                row["worker_attributes_computed"]["registration_platform"] = registration_platform
                row["worker_attributes_computed"]["registration_date"] = registration_date
                yield yt.create_table_switch(row["_table_index"])
                del row["_table_index"], row["worker_id"]
                yield row


def find_existing_tables(tables, client):
    """
    :type tables: list[str]
    :type client: yt.YtClient
    :rtype: __generator[str]
    """
    return (table for table in tables if yt.exists(table, client=client))


def find_dst_indexes(destinations):
    """
    :type destinations: dict[ExperimentForSqueeze, str]
    :rtype: (list[ExperimentForSqueeze], list[str], dict[str, int])
    """
    sorted_exps = sorted(destinations, key=lambda e: e.testid)
    tables = [destinations[exp] for exp in sorted_exps]
    table_indexes = {exp: i for i, exp in enumerate(sorted_exps)}
    return sorted_exps, tables, table_indexes


def generate_tmp(name):
    name = name.strip("/").replace("/", "__").replace(".", "_")
    return "mstand.{}.{}".format(name, uuid.uuid4())


def prepare_tmp_tables(dst_tables, client, attributes):
    """
    :type dst_tables: list[str]
    :type client: yt.YtClient
    :type attributes: list[dict]
    :rtype: list[str]
    """
    if attributes is None:
        attributes = [None for _ in dst_tables]

    if "DEBUG_TMP_DIR" in os.environ:
        root = os.environ["DEBUG_TMP_DIR"]
        logging.info("Will use %s tmp folder", root)
    else:
        root = "//tmp"
    prefixes = [generate_tmp(dst) for dst in dst_tables]
    prefixes_and_attributes = zip(prefixes, attributes)

    return [yt.create_temp_table(root, prefix, client=client, attributes=attribute) for prefix, attribute
            in prefixes_and_attributes]


def sort_wrapper(tmp, yt_pool, client, sort_by_columns, transform=True):
    """
    :type tmp: str
    :type yt_pool: str | None
    :type client: yt.YtClient
    :type sort_by_columns: list[str] | None
    :type transform: bool
    """
    yt_spec = yt_io.get_yt_operation_spec(title="mstand sort and transform squeeze {}".format(tmp),
                                          yt_pool=yt_pool,
                                          use_porto_layer=False)

    if transform:
        yt.set(tmp + "/@compression_codec", "brotli_5", client=client)
        yt.set(tmp + "/@erasure_codec", "lrc_12_2_2", client=client)

    return yt.run_sort(
        tmp,
        sort_by=sort_by_columns,
        spec=yt_spec,
        client=client,
        sync=False,
    )


def sort_result_tables(tmp_tables, client, sort_threads, sort_by_columns=None, yt_pool=None, transform=True):
    """
    :type tmp_tables: list[str]
    :type client: yt.YtClient
    :type sort_threads: int
    :type sort_by_columns: list[str] | None
    :type yt_pool: str | None
    :type transform: bool
    """
    if sort_by_columns is None:
        sort_by_columns = ["yuid", "ts", "action_index"]
    if transform:
        logging.info(
            "Will sort and transform temporary tables using %s parallel operations: %s",
            sort_threads,
            umisc.to_lines(tmp_tables),
        )
    else:
        logging.info(
            "Will sort temporary tables using %s parallel operations:%s",
            sort_threads,
            umisc.to_lines(tmp_tables),
        )
    time_start = time.time()
    worker = functools.partial(
        sort_wrapper,
        client=client,
        sort_by_columns=sort_by_columns,
        yt_pool=yt_pool,
        transform=transform,
    )
    mstand_uyt.async_yt_operations_poll(worker, tmp_tables, max_threads=sort_threads)
    time_end = time.time()
    count = len(tmp_tables)
    if transform:
        logging.info(
            "%d tables sorted and transformed in %s: %s",
            count,
            datetime.timedelta(seconds=time_end - time_start),
            umisc.to_lines(tmp_tables),
        )
    else:
        logging.info(
            "%d tables sorted in %s: %s",
            count,
            datetime.timedelta(seconds=time_end - time_start),
            umisc.to_lines(tmp_tables),
        )


def set_versions(exps, tables, versions, client, squeeze_way):
    """
    :type exps: list[ExperimentForSqueeze]
    :type tables: list[str]
    :type versions: SqueezeVersions
    :type client: yt.YtClient
    :type squeeze_way: SqueezeWayEnum
    """
    for exp, table in zip(exps, tables):
        with_history = exp.all_for_history
        with_filters = bool(exp.observation.filters)
        table_versions = versions.clone(
            service=exp.service,
            with_history=with_history,
            with_filters=with_filters,
            squeeze_way=squeeze_way,
        )
        versions_yt.set_versions(table, table_versions, client)


def set_cache_versions(exps, tables, cache_versions, client):
    """
    :type exps: list[ExperimentForSqueeze]
    :type tables: list[str]
    :type cache_versions: SqueezeVersions
    :type client: yt.YtClient
    """
    for exp, table in zip(exps, tables):
        versions_yt.set_versions(table, cache_versions, client, from_cache=True)


def set_cache_filters(exps, tables, cache_filters, client):
    """
    :type exps: list[ExperimentForSqueeze]
    :type tables: list[str]
    :type cache_filters: dict[str]
    :type client: yt.YtClient
    """
    if not cache_filters:
        return
    logging.info("Set cache_filters attribute")
    for exp, table in zip(exps, tables):
        if exp.service == ServiceEnum.YUID_REQID_TESTID_FILTER:
            yt.set_attribute(table, "cache_filters", list(cache_filters), client=client)


def separate_empty_tables(tmp_tables, dst_tables, client):
    """
    :type tmp_tables: list[str]
    :type dst_tables: list[str]
    :type client: yt.YtClient
    :rtype: tuple(list[str], list[str], list[str], list[str])
    """
    empty_tables = []
    empty_tables_dst = []
    non_empty_tables = []
    non_empty_tables_dst = []
    for tmp, dst in zip(tmp_tables, dst_tables):
        if yt.is_empty(tmp, client=client):
            empty_tables.append(tmp)
            empty_tables_dst.append(dst)
        else:
            non_empty_tables.append(tmp)
            non_empty_tables_dst.append(dst)
    return empty_tables, empty_tables_dst, non_empty_tables, non_empty_tables_dst


def move_result_tables(tmp_tables, dst_tables, client):
    """
    :type tmp_tables: list[str]
    :type dst_tables: list[str]
    :type client: yt.YtClient
    """
    logging.info(
        "Will move temporary tables %s\nto destination %s",
        umisc.to_lines(tmp_tables),
        umisc.to_lines(dst_tables),
    )
    for tmp, dst in zip(tmp_tables, dst_tables):
        yt.move(
            tmp,
            dst,
            force=True,
            preserve_account=False,
            preserve_expiration_time=False,
            client=client,
        )


def complete_squeeze(tmp_tables, dst_tables, client, allow_empty_tables=False):
    """
    :type tmp_tables: list[str]
    :type dst_tables: list[str]
    :type client: yt.YtClient
    :type allow_empty_tables: bool
    :rtype bool
    """
    empty_tables, empty_tables_dst, non_empty_tables, non_empty_tables_dst = \
        separate_empty_tables(tmp_tables, dst_tables, client)
    if empty_tables:
        logging.warning("Following tables are empty: %s", umisc.to_lines(empty_tables_dst))
    if allow_empty_tables:
        move_result_tables(tmp_tables, dst_tables, client)
        return False
    else:
        move_result_tables(non_empty_tables, non_empty_tables_dst, client)
        if empty_tables:
            for empty in empty_tables:
                yt.remove(
                    empty,
                    force=True,
                    client=client,
                )
            return True
        else:
            return False


def yt_spec_generator(squeezer,
                      yt_job_options,
                      data_size_per_job,
                      title,
                      add_acl):
    """
    :type squeezer: session_squeezer.squeezer.UserSessionsSqueezer
    :type yt_job_options: YtJobOptions
    :type data_size_per_job: int
    :type title: str
    :type add_acl: bool
    :rtype dict
    """
    if not title:
        title = "mstand squeeze {} {}".format(squeezer.day, sorted(squeezer.experiments))
    else:
        title = "mstand {} squeeze {} {}".format(title, squeezer.day, sorted(squeezer.experiments))

    return yt_io.get_yt_operation_spec(title=title,
                                       max_failed_job_count=5,
                                       data_size_per_job=data_size_per_job,
                                       data_size_per_map_job=data_size_per_job,
                                       partition_data_size=data_size_per_job,
                                       memory_limit=yt_job_options.memory_limit * yt_common.MB,
                                       operation_executor_types=YtOperationTypeEnum.MAP_REDUCE,
                                       enable_input_table_index=True,
                                       check_input_fully_consumed=True,
                                       max_row_weight=128 * yt_common.MB,
                                       use_porto_layer=not usix.is_arcadia(),
                                       yt_pool=yt_job_options.pool,
                                       tentative_job_spec=yt_job_options.get_tentative_job_spec(),
                                       acl=mstand_uyt.get_online_yt_operation_acl() if add_acl else None)


def get_attributes_with_schema(sorted_exps):
    columns_descriptions = [
        squeezer_services.get_columns_description_by_service(sorted_exp.service)
        for sorted_exp in sorted_exps
    ]
    schemas = [yt_yson.YsonList(columns_description) for columns_description in columns_descriptions]
    for schema in schemas:
        schema.attributes["strict"] = True
    return [{"schema": schema} for schema in schemas]


def get_attributes_with_schema_from_cache(sorted_exps, client, from_cache_day):
    schemas = [
        yt.get_attribute(
            mstand_tables.CacheTable(
                day=from_cache_day, source=ServiceEnum.get_cache_source(sorted_exp.service)
            ).path,
            "schema",
            client=client
        )
        for sorted_exp in sorted_exps
    ]
    for schema in schemas:
        schema.attributes["strict"] = True
    return [{"schema": schema} for schema in schemas]


def get_tmp_tables(sorted_exps, dst_tables, client, from_cache_day=None, without_schema=False):
    if without_schema:
        attributes_with_schema = None
    elif from_cache_day:
        attributes_with_schema = get_attributes_with_schema_from_cache(sorted_exps, client, from_cache_day)
    else:
        attributes_with_schema = get_attributes_with_schema(sorted_exps)

    tmp_tables = prepare_tmp_tables(
        dst_tables=dst_tables,
        client=client,
        attributes=attributes_with_schema,
    )
    return tmp_tables


def get_tmp_tables_objects(sorted_exps, dst_tables, client):
    tmp_tables = get_tmp_tables(sorted_exps, dst_tables, client)
    tmp_tables_objects = [yt.TablePath(
        table,
        append=True,
        client=client,
    ) for table in tmp_tables]
    return tmp_tables, tmp_tables_objects


def optimize_data_size_per_job(data_size_per_job, experiment_number):
    """
    :type data_size_per_job: int
    :type experiment_number: int
    :rtype int
    """
    if experiment_number > 50:
        return data_size_per_job // 3
    elif experiment_number > 10:
        return data_size_per_job // 2
    else:
        return data_size_per_job


class ServiceParams:
    def __init__(self,
                 destinations: Dict[ExperimentForSqueeze, str],
                 squeezer: UserSessionsSqueezer,
                 paths: SqueezePaths,
                 local_files: YTBackendLocalFiles,
                 versions: SqueezeVersions,
                 cache_versions: SqueezeVersions,
                 client: yt.YtClient,
                 yt_job_options: YtJobOptions,
                 transform: bool,
                 table_bounds: TableBounds,
                 allow_empty_tables: bool,
                 sort_threads: int,
                 day: datetime.date,
                 add_acl: bool,
                 operation_sid: str,
                 squeeze_backend: "SqueezeBackendYT",
                 squeeze_bin_file: str) -> None:
        # TODO: fix squeeze_backend annotation with __future__.annotations in python 3.7
        self.destinations = destinations
        self.squeezer = squeezer
        self.paths = paths
        self.local_files = local_files
        self.versions = versions
        self.cache_versions = cache_versions
        self.client = client
        self.yt_job_options = yt_job_options
        self.transform = transform
        self.table_bounds = table_bounds
        self.allow_empty_tables = allow_empty_tables
        self.sort_threads = sort_threads
        self.day = day
        self.add_acl = add_acl
        self.operation_sid = operation_sid
        self.squeeze_backend = squeeze_backend
        self.squeeze_bin_file = squeeze_bin_file


@mstand_uyt.retry_yt_operation_default
def reduce_day(params: ServiceParams) -> None:
    if params.squeezer.source == ServiceSourceEnum.SEARCH_STAFF:
        # uu/*
        lower_key = "is"
        upper_key = "z"
        data_size_per_job = 100 * yt_common.MB
    elif params.squeezer.source == ServiceSourceEnum.WATCHLOG:
        lower_key = "y"
        upper_key = "z"
        data_size_per_job = 2 * yt_common.GB
    else:
        # uu/ + y*
        lower_key = "uu/"  # MSTAND-1130
        upper_key = "z"
        data_size_per_job = 12 * yt_common.GB

    if params.table_bounds.lower_reducer_key is not None:
        lower_key = params.table_bounds.lower_reducer_key
    if params.table_bounds.upper_reducer_key is not None:
        upper_key = params.table_bounds.upper_reducer_key

    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)

    # use more jobs for heavy calculations
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    session_key = "key"
    session_subkey = "subkey"
    reduce_sort_by = [session_key, session_subkey]

    source_tables = []
    for table_type, table_path in zip(params.paths.types, params.paths.tables):
        if table_type == TableTypeEnum.YUID:
            sorted_by = [session_key]
        elif table_type == TableTypeEnum.SOURCE:
            sorted_by = [session_key, session_subkey]
        else:
            raise Exception("Got unexpected table type: %s", table_type)

        source_tables.append(yt.TablePath(
                             table_path,
                             sorted_by=sorted_by,
                             lower_key=lower_key,
                             upper_key=upper_key,
                             client=params.client,))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="",
                                add_acl=params.add_acl)

    core_table = None  # "//home/mstand/.test/core_2"
    if core_table:
        yt.create("table", core_table, recursive=True, client=params.client)
        yt_spec["core_table_path"] = core_table
        yt_spec["max_failed_job_count"] = 100

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_REDUCE_START,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        try:
            op = yt.run_reduce(
                SqueezeYTReducer(params.squeezer, dst_indexes,
                                 key=session_key,
                                 table_types=params.paths.types),
                source_table=source_tables,
                destination_table=tmp_tables,
                local_files=params.local_files.files,
                yt_files=params.local_files.yt_files,
                reduce_by=[session_key],
                sort_by=reduce_sort_by,
                spec=yt_spec,
                client=params.client,
                format=yt.YsonFormat(control_attributes_mode="row_fields")
            )
        except yt.YtOperationFailedError as e:
            if "stderrs" in e.attributes:
                e.message = e.message + "\n" + yt.format_operation_stderrs(e.attributes["stderrs"])
            raise
        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_REDUCE_FINISH,
            data=dict(
                operation_sid=params.operation_sid,
                job_statistics=op.get_job_statistics(),
                operation_id=op.id,
                operation_url=op.url,
                operation_type=op.type,
            )
        )

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_SORT_START,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_SORT_FINISH,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        set_cache_filters(sorted_exps, tmp_tables, params.squeezer.raw_cache_filters, params.client)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


def prepare_whitelist_table(
        params: ServiceParams,
        yt_spec: dict,
        lower_key: Optional[str],
        upper_key: Optional[str],
        transaction_id: str,
) -> str:
    import libra
    from nile.api.v1 import (
        clusters,
        Record,
    )

    cluster = clusters.yt.Hahn()
    job = cluster \
        .job("prepare whitelist") \
        .env(
            yt_spec_defaults=yt_spec,
            merge_strategy=dict(final_tables="never"),
        )

    def reducer(groups):
        for key, rows in groups:
            key_is_ok = False
            for row in rows:
                row_testids = set(row["value"].decode().split("\t"))
                key_is_ok |= bool(row_testids.intersection(params.squeezer.testids))
            if key_is_ok:
                yield Record(key, subkey="", value=libra.GetValueForFilteredProtocol())

    yuid_paths = params.paths.yuids
    logging.info("Got %d yuid_testids tables:\n\t%s", len(yuid_paths), "\n\t".join(yuid_paths))
    if lower_key and upper_key:
        yuid_paths = [f"{path}[{lower_key}:{upper_key}]" for path in yuid_paths]
    yuid_paths = [job.table(path) for path in yuid_paths]

    whitelist_table = params.client.create_temp_table("//tmp", "mstand_squeeze_whitelist_")

    job \
        .concat(*yuid_paths) \
        .groupby("key") \
        .reduce(reducer) \
        .sort("key", "subkey") \
        .put(whitelist_table)

    with job.driver.transaction(transaction_id=transaction_id):
        job.run()

    return whitelist_table


def _get_yt_reduce_operation(operation_ids: Tuple[str], client: yt.YtClient) -> Optional[yt.Operation]:
    for operation_id in operation_ids:
        op_type = yt.operation_commands.get_operation(operation_id, ["type"], client=client).get("type")
        if op_type == "reduce":
            return yt.Operation(id=operation_id, type=op_type, client=client)


@mstand_uyt.retry_yt_operation_default
def reduce_day_nile(params: ServiceParams) -> None:
    from nile.api.v1 import (
        clusters,
        files as nfi,
        with_hints,
    )

    data_size_per_job = 12 * yt_common.GB
    lower_key = params.table_bounds.lower_reducer_key
    upper_key = params.table_bounds.upper_reducer_key

    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)

    # use more jobs for heavy calculations
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client) as t:
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.NILE_REDUCE_START,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        cluster = clusters.yt.Hahn()
        job = cluster \
            .job("squeeze web") \
            .env(
                yt_spec_defaults=yt_spec,
                merge_strategy=dict(final_tables="never"),
            )

        if lower_key and upper_key:
            paths = [job.table(f"{path}[{lower_key}:{upper_key}]") for path in params.paths.sources]
        else:
            paths = [job.table(path) for path in params.paths.sources]

        reducer = with_hints(outputs_count=len(tmp_tables))(
            SqueezeNileReducer(params.squeezer, dst_indexes)
        )

        enable_whitelist_protocol = (
            all(not exp.all_users for exp in params.squeezer.experiments)
            and bool(params.paths.yuids)
        )
        if enable_whitelist_protocol:
            whitelist_table = prepare_whitelist_table(params, yt_spec, lower_key, upper_key, t.transaction_id)
            paths.append(job.table(whitelist_table))

        streams = job \
            .concat(*paths) \
            .libra(
                reducer,
                blockstat_dict_file=nfi.StatboxDict("blockstat.dict", use_latest=True),
                files=[
                    nfi.RemoteFile(path)
                    for path in params.local_files.yt_files
                    if not path.endswith("blockstat.dict")
                ],
                memory_limit=params.yt_job_options.memory_limit,
                enable_whitelist_protocol=enable_whitelist_protocol,
            )

        if len(tmp_tables) == 1:
            streams = [streams]

        for stream, tmp_table in zip(streams, tmp_tables):
            stream.put(tmp_table)

        with job.driver.transaction(transaction_id=t.transaction_id):
            run_info = job.run()

        op = _get_yt_reduce_operation(operation_ids=run_info.yt_operation_ids, client=params.client)
        if op is not None:
            params.squeeze_backend.write_log(
                rectype=RectypeEnum.NILE_REDUCE_FINISH,
                data=dict(
                    operation_sid=params.operation_sid,
                    job_statistics=op.get_job_statistics(),
                    operation_id=op.id,
                    operation_url=op.url,
                    operation_type=op.type,
                )
            )
        else:
            logging.warning("Reduce operation was not found: %r", run_info.yt_operation_ids)
            params.squeeze_backend.write_log(
                rectype=RectypeEnum.NILE_REDUCE_FINISH,
                data=dict(
                    operation_sid=params.operation_sid,
                )
            )

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_SORT_START,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_SORT_FINISH,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.NILE)
        set_cache_filters(sorted_exps, tmp_tables, params.squeezer.raw_cache_filters, params.client)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


def get_resource(path: str) -> str:
    if usix.is_arcadia():
        from library.python import resource
        key = os.path.join("resfs/file/tools/mstand", path)
        return resource.find(key).decode()
    else:
        import project_root
        return ufile.read_text_file(project_root.get_project_path(path))


def get_yql_parameters(
        params: ServiceParams,
        sorted_exps: List[ExperimentForSqueeze],
        tmp_tables: List[str],
        transaction_id: str,
) -> JsonDict:
    from yql.client.parameter_value_builder import YqlParameterValueBuilder as ValueBuilder
    experiments = [
        ValueBuilder.make_struct(
            testid=ValueBuilder.make_string(efs.testid),
            tmp_table_path=ValueBuilder.make_string(tmp_table),
        )
        for efs, tmp_table in zip(sorted_exps, tmp_tables)
    ]

    yt_pool = params.yt_job_options.pool or params.client.get_user_name()

    parameters = {
        "$yt_pool": ValueBuilder.make_string(yt_pool),
        "$day": ValueBuilder.make_date(params.day),
        "$cluster": ValueBuilder.make_string(params.client.config["proxy"]["url"]),
        "$transaction_id": ValueBuilder.make_string(transaction_id),
        "$experiments": ValueBuilder.make_list(experiments),
    }
    return ValueBuilder.build_json_map(parameters)


def get_yql_op_statistics(operation_id: str) -> Optional[JsonDict]:
    from yql.client.operation import YqlOperationPlanRequest
    statistics_request = YqlOperationPlanRequest(operation_id, statistics=True)
    statistics_request.run()
    if statistics_request.is_ok:
        return statistics_request.json.get("statistics")
    else:
        logging.warning("%s", statistics_request.exc_info)


@mstand_uyt.retry_yt_operation_default
def reduce_day_using_yql(params: ServiceParams) -> None:
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)

    with yt.Transaction(client=params.client) as t:
        tmp_tables = get_tmp_tables(
            sorted_exps=sorted_exps,
            dst_tables=dst_tables,
            client=params.client,
            without_schema=True,
        )

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YQL_REDUCE_START,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        from yql.api.v1.client import YqlClient
        client = YqlClient(token_path=params.squeeze_backend.yql_token_file)
        request = client.query(
            query=get_resource("yql_scripts/zen/run_squeeze.sql"),
            title="squeeze experiments using YQL: {}".format([(exp.service, exp.testid) for exp in sorted_exps]),
        )
        request.attach_file_data(data=get_resource("yql_scripts/zen/squeeze_lib.sql"), name="squeeze_lib.sql")
        request.run(parameters=get_yql_parameters(params, sorted_exps, tmp_tables, t.transaction_id))

        results = request.get_results()
        assert results.is_ok and results.is_success

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YQL_REDUCE_FINISH,
            data=dict(
                operation_sid=params.operation_sid,
                shared_url=request.share_url,
                yql_statistics=get_yql_op_statistics(request.operation_id),
            )
        )

        if params.transform:
            params.squeeze_backend.write_log(
                rectype=RectypeEnum.YT_SORT_START,
                data=dict(
                    operation_sid=params.operation_sid,
                )
            )

            sort_result_tables(tmp_tables,
                               params.client,
                               params.sort_threads,
                               yt_pool=params.yt_job_options.pool,
                               transform=params.transform)

            params.squeeze_backend.write_log(
                rectype=RectypeEnum.YT_SORT_FINISH,
                data=dict(
                    operation_sid=params.operation_sid,
                )
            )

        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YQL)
        set_cache_filters(sorted_exps, tmp_tables, params.squeezer.raw_cache_filters, params.client)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def reduce_day_using_bin(params: ServiceParams) -> None:
    data_size_per_job = 6 * yt_common.GB  # TODO: Move into dict for different sources

    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)

    # use more jobs for heavy calculations
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    with yt.Transaction(client=params.client) as t:
        tmp_tables = get_tmp_tables(
            sorted_exps=sorted_exps,
            dst_tables=dst_tables,
            client=params.client,
            without_schema=True,
        )

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.BIN_REDUCE_START,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        squeeze_params = build_squeeze_params(
            params=params,
            data_size_per_job=data_size_per_job,
            transaction_id=t.transaction_id,
            dst_indexes=dst_indexes,
            tmp_tables=tmp_tables,
            max_data_size_per_job=params.yt_job_options.max_data_size_per_job * yt_common.GB,
        )

        operation_id = run_bin_squeeze(params.squeeze_bin_file, squeeze_params)
        op = _get_yt_reduce_operation(operation_ids=[operation_id], client=params.client)
        if op is not None:
            params.squeeze_backend.write_log(
                rectype=RectypeEnum.BIN_REDUCE_FINISH,
                data=dict(
                    operation_sid=params.operation_sid,
                    job_statistics=op.get_job_statistics(),
                    operation_id=op.id,
                    operation_url=op.url,
                    operation_type=op.type,
                )
            )
        else:
            logging.warning("Reduce operation was not found: %r", operation_id)
            params.squeeze_backend.write_log(
                rectype=RectypeEnum.BIN_REDUCE_FINISH,
                data=dict(
                    operation_sid=params.operation_sid,
                )
            )

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_SORT_START,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_SORT_FINISH,
            data=dict(
                operation_sid=params.operation_sid,
            )
        )

        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.BINARY)
        set_cache_filters(sorted_exps, tmp_tables, params.squeezer.raw_cache_filters, params.client)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


def build_squeeze_params(
        params: ServiceParams,
        data_size_per_job: int,
        transaction_id: str,
        dst_indexes: Dict[str, int],
        tmp_tables: List[str],
        max_data_size_per_job: int = 200,
) -> JsonDict:
    yt_params = {
        "AddAcl": params.add_acl,
        "TentativeEnable": params.yt_job_options.tentative_enable,
        "SourcePaths": params.paths.sources,
        "YtFiles": params.local_files.yt_files,
        "YuidPaths": params.paths.yuids,
        "LowerKey": params.table_bounds.lower_reducer_key,
        "Pool": params.yt_job_options.pool,
        "Server": params.client.config["proxy"].get("url", ""),
        "TransactionId": transaction_id,
        "UpperKey": params.table_bounds.upper_reducer_key,
        "DataSizePerJob": data_size_per_job,
        "MaxDataSizePerJob": max_data_size_per_job,
        "MemoryLimit": params.yt_job_options.memory_limit * yt_common.MB,
    }

    experiments = []
    for (efs, i), tmp_table in zip(dst_indexes.items(), tmp_tables):
        d_filters = [
            {"Name": name, "Value": value}
            for name, value in efs.filters.libra_filters
        ]

        d_efs = {
            "Day": utime.format_date(params.day),
            "FilterHash": efs.filter_hash,
            "Filters": d_filters,
            "Service": efs.service,
            "TableIndex": i,
            "TempTablePath": tmp_table,
            "Testid": efs.testid,
            "IsHistoryMode": efs.all_for_history,
        }

        experiments.append(d_efs)

    filters = {}
    for uid, fltr in (params.squeezer.raw_cache_filters or {}).items():
        filters[uid] = [{"Name": n, "Value": v} for n, v in fltr.libra_filters]

    return {
        "YtParams": yt_params,
        "Experiments": experiments,
        "RawUid2Filter": filters,
    }


def run_bin_squeeze(squeeze_bin_file: str, squeeze_params: JsonDict) -> str:
    logging.info("Run squeeze using bin file %s", squeeze_bin_file)

    temp_dir = ufile.create_temp_dir(prefix="", subdir="mstand_bin_squeeze")
    input_json = os.path.join(temp_dir, "input.json")
    output_json = os.path.join(temp_dir, "output.json")

    ujson.dump_to_file(squeeze_params, path=input_json, pretty=True)

    cmd = [
        squeeze_bin_file,
        "--mode", "yt",
        "--squeeze-params", input_json,
        "--output", output_json,
    ]
    umisc.run_command(cmd=cmd, log_command_line=True)

    result = ujson.load_from_file(path=output_json)

    assert "operation_id" in result
    return result["operation_id"]


@mstand_uyt.retry_yt_operation_default
def prepare_daily_for_yuids_merge(
        # destinations: Dict[ExperimentForSqueeze, str],
        squeezer: UserSessionsSqueezer,
        source_path: str,
        local_files: YTBackendLocalFiles,
        client: yt.YtClient,
        yt_job_options: YtJobOptions,
        table_bounds: TableBounds,
        sort_threads: int,
        yt_files: Optional[List[str]],
        mapper: Callable[[Any], Generator[Any, None, None]],
        add_acl: bool,
):
    # sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=destinations)

    start_index = table_bounds.start_mapper_index
    end_index = table_bounds.end_mapper_index

    sessions_table = yt.TablePath(
        source_path,
        sorted_by=["key", "subkey"],
        client=client,
        start_index=start_index,
        end_index=end_index,
    )

    data_size_per_job = yt_common.GB
    yt_spec = yt_spec_generator(squeezer=squeezer,
                                yt_job_options=yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="",
                                add_acl=add_acl)

    with yt.Transaction(client=client):
        prefix = generate_tmp(source_path)
        tmp_table = yt.create_temp_table("//tmp", prefix, client=client)

        try:
            yt.run_map(
                mapper,
                source_table=sessions_table,
                destination_table=tmp_table,
                local_files=local_files.files,
                yt_files=yt_files,
                spec=yt_spec,
                client=client,
                format=yt.YsonFormat(control_attributes_mode="row_fields"),
            )
        except yt.YtOperationFailedError as e:
            if "stderrs" in e.attributes:
                e.message = e.message + "\n" + yt.format_operation_stderrs(e.attributes["stderrs"])
            raise
        sort_result_tables([tmp_table], client, sort_threads,
                           sort_by_columns=["key", "subkey"], yt_pool=yt_job_options.pool)

    return tmp_table


def _dummy_mapper(row):
    yield row


@mstand_uyt.retry_yt_operation_default
def prepare_toloka(params: ServiceParams) -> None:

    # TODO: merge with reduce_day

    data_size_per_job = 40 * yt_common.GB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    # use more jobs for heavy calculations
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    # TODO: better design
    assert len(params.paths.sources) == 5
    path_ep, path_wps, path_rv56, path_worker_source, path_worker_daily_statistics = params.paths.sources

    assert "/eligible_pools/" in path_ep
    assert "/worker_pool_selection/" in path_wps
    assert "/results_v56" in path_rv56
    assert path_worker_source.endswith("/worker_source")
    assert "/worker_daily_statistics/" in path_worker_daily_statistics

    source_ep = yt.TablePath(path_ep, client=params.client)
    source_wps = yt.TablePath(path_wps, client=params.client)
    source_worker_daily_statistics = yt.TablePath(path_worker_daily_statistics, client=params.client)

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="toloka",
                                add_acl=params.add_acl)

    tmp_tables, tmp_tables_objects = get_tmp_tables_objects(sorted_exps=sorted_exps,
                                                            dst_tables=dst_tables,
                                                            client=params.client)

    rv56_query = ActionsSqueezerToloka.rv56_query(params.squeezer.day)
    yt_spec_rv56 = dict(yt_spec, input_query=rv56_query)

    # Dynamic table commands can not be performed under master transaction
    # yt.select_rows(query=rv56_query, client=client)
    with yt.TempTable() as tmp_rv56_filter:
        yt.run_map(_dummy_mapper, path_rv56, tmp_rv56_filter, spec=yt_spec_rv56, client=params.client)
        yt.run_map_reduce(
            SqueezeYTTolokaJoinMapper(params.squeezer, dst_indexes, toloka_table="rv56"),
            SqueezeYTTolokaJoinReducer(),
            source_table=[tmp_rv56_filter, path_worker_source],
            destination_table=tmp_tables_objects,
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="iterator"),
            reduce_by="worker_id",
            sort_by=["worker_id", "_table_index"],
        )

    with yt.Transaction(client=params.client):
        try:
            yt.run_map_reduce(
                SqueezeYTTolokaJoinMapper(params.squeezer, dst_indexes, toloka_table="ep"),
                SqueezeYTTolokaJoinReducer(),
                source_table=[source_ep, path_worker_source],
                destination_table=tmp_tables_objects,
                spec=yt_spec,
                client=params.client,
                format=yt.YsonFormat(control_attributes_mode="iterator"),
                reduce_by="worker_id",
                sort_by=["worker_id", "_table_index"],
            )

            yt.run_map_reduce(
                SqueezeYTTolokaJoinMapper(params.squeezer, dst_indexes, toloka_table="wps"),
                SqueezeYTTolokaJoinReducer(),
                source_table=[source_wps, path_worker_source],
                destination_table=tmp_tables_objects,
                spec=yt_spec,
                client=params.client,
                format=yt.YsonFormat(control_attributes_mode="iterator"),
                reduce_by="worker_id",
                sort_by=["worker_id", "_table_index"],
            )

            yt.run_map(
                SqueezeYTTolokaMapper(params.squeezer, dst_indexes, toloka_table="worker_daily_statistics"),
                source_table=source_worker_daily_statistics,
                destination_table=tmp_tables_objects,
                spec=yt_spec,
                client=params.client,
                format=yt.YsonFormat(control_attributes_mode="row_fields"),
            )
        except yt.YtOperationFailedError as e:
            if "stderrs" in e.attributes:
                e.message = e.message + "\n" + yt.format_operation_stderrs(e.attributes["stderrs"])
            raise
        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_alice(params: ServiceParams) -> None:

    # TODO: merge with reduce_day
    data_size_per_job = 4 * yt_common.GB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.sources) <= 2
    source = [yt.TablePath(table, client=params.client) for table in params.paths.sources]

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="alice",
                                add_acl=params.add_acl)

    def mapper(row):
        if row["@table_index"] == 0:
            row["yuid"] = row["uuid"]
            row["ts"] = row["client_time"]
            del row["@table_index"]
            yield row
        elif row["uuid"] is not None:
            if row["event_name"] == "progressHeartbeatMusic":
                generic_scenario = "heartbeat_music"
            elif row["event_name"] == "progressHeartbeatRadio":
                generic_scenario = "heartbeat_radio"
            elif row["event_name"] == "progressHeartbeatVideo":
                generic_scenario = "heartbeat_video"
            else:
                generic_scenario = "heartbeat_other"

            other = {
                "device_id": row["device_id"],
                "event_name": row["event_name"],
                "percent_played": row["percent_played"],
                "provider": row["provider"],
                "track_id": row["track_id"],
                "video_url": row["video_url"],
            }

            yield {
                "yuid": row["uuid"],
                "ts": row["send_timestamp"],
                "fielddate": row["dt"],
                "generic_scenario": generic_scenario,
                "input_type": "heartbeat",
                "other": other,
                "req_id": row["req_id"],
            }

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        yt.run_map_reduce(
            mapper=mapper,
            reducer=SqueezeYTReducer(params.squeezer, dst_indexes, key="yuid", table_types=[TableTypeEnum.SOURCE]),
            source_table=source,
            destination_table=tmp_tables,
            reduce_by="yuid",
            sort_by=["yuid", "ts"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_turbo(params: ServiceParams) -> None:

    # TODO: merge with reduce_day
    data_size_per_job = 512 * yt_common.MB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.sources) == 1

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="turbo",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        def mapper(row):
            if row["yuid"] is not None:
                row["yuid"] = str(row["yuid"])
                yield row

        yt.run_map_reduce(
            mapper,
            SqueezeYTReducer(params.squeezer, dst_indexes, key="yuid", table_types=params.paths.types),
            source_table=params.paths.sources,
            destination_table=tmp_tables,
            reduce_by="yuid",
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_ether(params: ServiceParams) -> None:

    # TODO: merge with reduce_day
    data_size_per_job = 512 * yt_common.MB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.sources) == 1

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="ether",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        yt.run_map_reduce(
            ether_mapper,
            SqueezeYTReducer(params.squeezer, dst_indexes, key="yuid", table_types=params.paths.types),
            source_table=params.paths.sources,
            destination_table=tmp_tables,
            reduce_by="yuid",
            sort_by=["yuid", "ts"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_ya_video(params: ServiceParams) -> None:

    # TODO: merge with reduce_day
    data_size_per_job = 512 * yt_common.MB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.sources) == 1

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="ya video",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        yt.run_map_reduce(
            ya_video_mapper,
            SqueezeYTReducer(params.squeezer, dst_indexes, key="yuid", table_types=params.paths.types),
            source_table=params.paths.sources,
            destination_table=tmp_tables,
            reduce_by="yuid",
            sort_by=["yuid", "ts"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_object_answer(params: ServiceParams) -> None:

    # TODO: merge with reduce_day
    data_size_per_job = 512 * yt_common.MB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.sources) == 1

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="object answer",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        yt.run_map_reduce(
            None,
            SqueezeYTReducer(params.squeezer, dst_indexes, key="UID", table_types=params.paths.types),
            source_table=params.paths.sources,
            destination_table=tmp_tables,
            reduce_by="UID",
            sort_by=["UID", "Timestamp"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_pp(params: ServiceParams) -> None:

    # TODO: merge with reduce_day
    data_size_per_job = 512 * yt_common.MB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.sources) == 1
    # TODO: use PpTable.ranges
    ranges = [
        {"exact": {"key": [app_id, "VERTICAL_SWITCH", "HEADER_CLICK"]}}
        for app_id in ("ru.yandex.searchplugin", "ru.yandex.searchplugin.beta")
    ]
    source = yt.TablePath(
        name=params.paths.sources[0],
        ranges=ranges,
        client=params.client,
    )

    yt_spec = yt_spec_generator(
        squeezer=params.squeezer,
        yt_job_options=params.yt_job_options,
        data_size_per_job=data_size_per_job,
        title="pp",
        add_acl=params.add_acl,
    )

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(
            sorted_exps=sorted_exps,
            dst_tables=dst_tables,
            client=params.client,
        )

        def mapper(row):
            yuid = "y{}".format(row["icookie"])
            row_params = row["params"]
            if (
                yuid in params.table_bounds
                and row_params
                and row_params.get("from") == "serp"
                and row_params.get("to") != "serp"
            ):
                yield dict(
                    yuid=yuid,
                    ts=row["timestamp"],
                    reqid=row_params.get("request_id"),
                    testids=row["test_ids"],
                )

        yt.run_map_reduce(
            mapper,
            SqueezeYTReducer(params.squeezer, dst_indexes, key="yuid", table_types=params.paths.types),
            source_table=source,
            destination_table=tmp_tables,
            reduce_by="yuid",
            sort_by=["yuid", "ts"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(
            tmp_tables=tmp_tables,
            client=params.client,
            sort_threads=params.sort_threads,
            yt_pool=params.yt_job_options.pool,
            transform=params.transform,
        )
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_surveys(params: ServiceParams) -> None:

    # TODO: merge with reduce_day
    data_size_per_job = 512 * yt_common.MB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.yuids) > 0 or not params.paths.yuids and params.day == datetime.date(2022, 1, 27)
    assert len(params.paths.sources) == 1

    yt_spec = yt_spec_generator(
        squeezer=params.squeezer,
        yt_job_options=params.yt_job_options,
        data_size_per_job=data_size_per_job,
        title="surveys",
        add_acl=params.add_acl,
    )

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(
            sorted_exps=sorted_exps,
            dst_tables=dst_tables,
            client=params.client,
        )

        necessary_testids = params.squeezer.testids

        def mapper(row):
            row["table_index"] = row.pop("@table_index")
            if row["table_index"] < len(params.paths.yuids):
                testids = necessary_testids & set(row["value"].split("\t"))
                if testids:
                    row["value"] = "\t".join(testids)
                    yield row

            else:
                key = "y{}".format(row["yandexuid"])
                ts = row["question_timestamp"] or row["answer_timestamp"]
                if key != "y0" and ts is not None:
                    row["key"] = key
                    row["ts"] = ts
                    yield row

        source_table = yt.TablePath(
            name=params.paths.sources[0],
            exact_key=params.day.strftime("%Y-%m-%d"),
            client=params.client,
        )

        yt.run_map_reduce(
            mapper=mapper,
            reducer=SqueezeYTReducer(params.squeezer, dst_indexes, key="key", table_types=params.paths.types),
            source_table=params.paths.yuids + [source_table],
            destination_table=tmp_tables,
            reduce_by="key",
            sort_by=["key", "table_index", "question_timestamp"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(
            tmp_tables=tmp_tables,
            client=params.client,
            sort_threads=params.sort_threads,
            yt_pool=params.yt_job_options.pool,
            transform=params.transform,
        )
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_zen_surveys(params: ServiceParams) -> None:
    # TODO: merge with reduce_day
    data_size_per_job = 512 * yt_common.MB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.yuids) == 1
    assert len(params.paths.sources) == 2

    yt_spec = yt_spec_generator(
        squeezer=params.squeezer,
        yt_job_options=params.yt_job_options,
        data_size_per_job=data_size_per_job,
        title="zen-surveys",
        add_acl=params.add_acl,
    )

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(
            sorted_exps=sorted_exps,
            dst_tables=dst_tables,
            client=params.client,
        )

        def mapper(row):
            row["table_index"] = row.pop("@table_index")
            if row["table_index"] < 2:
                yield row
            else:
                ts = row["question_timestamp"] or row["answer_timestamp"]
                if ts is not None:
                    row["ts"] = ts
                    yield row

        tables = [
            yt.TablePath(
                params.paths.yuids[0],
                rename_columns={"key": "strongest_id"},
                client=params.client,
            ),
            yt.TablePath(
                params.paths.sources[0],
                columns=[
                    "experiments",
                    "history_user_temperature",
                    "integration",
                    "partner",
                    "product",
                    "strongest_id",
                    "user_temperature",
                ],
                rename_columns={
                    "strongestId": "strongest_id",
                    "userHistoryUserTemperatureV2": "history_user_temperature",
                    "userTemperature": "user_temperature",
                },
                client=params.client,
            ),
            yt.TablePath(
                name=params.paths.sources[1],
                exact_key=params.day.strftime("%Y-%m-%d"),
                client=params.client,
            ),
        ]

        yt.run_map_reduce(
            mapper=mapper,
            reducer=SqueezeYTReducer(params.squeezer, dst_indexes, key="strongest_id", table_types=params.paths.types),
            source_table=tables,
            destination_table=tmp_tables,
            reduce_by="strongest_id",
            sort_by=["strongest_id", "table_index", "question_timestamp", "answer_timestamp"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(
            tmp_tables=tmp_tables,
            client=params.client,
            sort_threads=params.sort_threads,
            yt_pool=params.yt_job_options.pool,
            transform=params.transform,
        )
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_vh(params: ServiceParams) -> None:
    # TODO: merge with reduce_day
    data_size_per_job = 512 * yt_common.MB
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    assert len(params.paths.yuids) == 0
    assert len(params.paths.sources) == 1

    yt_spec = yt_spec_generator(
        squeezer=params.squeezer,
        yt_job_options=params.yt_job_options,
        data_size_per_job=data_size_per_job,
        title="vh",
        add_acl=params.add_acl,
    )

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(
            sorted_exps=sorted_exps,
            dst_tables=dst_tables,
            client=params.client,
        )

        necessary_testids = params.squeezer.testids

        def mapper(row):
            if row["yandexuid"]:
                test_buckets = row.pop("test_buckets", "") or ""
                testids = necessary_testids & {val.split(",")[0] for val in test_buckets.split(";")}

                row["yandexuid"] = "y{}".format(row["yandexuid"])
                row["ts"] = row["timestamp"]
                row["testids"] = list(testids)
                yield row

        source_table = yt.TablePath(
            params.paths.sources[0],
            columns=["yandexuid", "timestamp", "view_time", "content_duration", "test_buckets"],
            client=params.client,
        )

        yt.run_map_reduce(
            mapper=mapper,
            reducer=SqueezeYTReducer(params.squeezer, dst_indexes, key="yandexuid", table_types=params.paths.types),
            source_table=source_table,
            destination_table=tmp_tables,
            reduce_by="yandexuid",
            sort_by=["yandexuid", "timestamp"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(
            tmp_tables=tmp_tables,
            client=params.client,
            sort_threads=params.sort_threads,
            yt_pool=params.yt_job_options.pool,
            transform=params.transform,
        )
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_app_metrics_toloka(params: ServiceParams) -> None:
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = 1 * yt_common.GB
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="app_metrics_toloka",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        def mapper(row):
            account_id = row["AccountID"]
            if account_id is not None and account_id != "0":
                row["worker_id"] = get_toloka_user_id(int(account_id))
                yield row

        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        source_table = yt.TablePath(
            name=params.paths.sources[0],
            exact_key="139535",  # toloka APIKey
        )

        yt.run_map_reduce(
            mapper=mapper,
            reducer=SqueezeYTReducer(params.squeezer, dst_indexes, key="worker_id", table_types=params.paths.types),
            source_table=source_table,
            destination_table=tmp_tables,
            reduce_by="worker_id",
            sort_by=["worker_id", "EventTimestamp"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
            reduce_local_files=params.local_files.files,
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_ya_metrics(params: ServiceParams) -> None:
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = 1 * yt_common.GB
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="ya_metrics",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        def mapper(row):
            passportuid = row["passportuid"]
            if passportuid is not None and passportuid != "0":
                row["worker_id"] = get_toloka_user_id(int(passportuid))
                row["ts"] = int(row["eventtime"])
                yield row

        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        source_table = yt.TablePath(
            name=params.paths.sources[0],
            exact_key="26573484",  # toloka counterid
        )

        yt.run_map_reduce(
            mapper=mapper,
            reducer=SqueezeYTReducer(params.squeezer, dst_indexes, key="worker_id", table_types=params.paths.types),
            source_table=source_table,
            destination_table=tmp_tables,
            reduce_by="worker_id",
            sort_by=["worker_id", "ts"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
            reduce_local_files=params.local_files.files,
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_ecom(params: ServiceParams) -> None:
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = 256 * yt_common.MB
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="ecom",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        yt.run_reduce(
            SqueezeYTReducer(params.squeezer, dst_indexes, key="key", table_types=params.paths.types),
            source_table=params.paths.tables,
            destination_table=tmp_tables,
            reduce_by="key",
            sort_by=["key"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields")
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_prism(params: ServiceParams) -> None:
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = 256 * yt_common.MB
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="prism",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        def mapper(row):
            if row["@table_index"] == 1:
                if row["yandexuid"] is None:
                    return
                row["key"] = "y" + row["yandexuid"]
            row["table_index"] = row.pop("@table_index")
            yield row

        yt.run_map_reduce(
            mapper,
            SqueezeYTReducer(params.squeezer, dst_indexes, key="key", table_types=[TableTypeEnum.SOURCE]),
            source_table=params.paths.tables,
            destination_table=tmp_tables,
            reduce_by="key",
            sort_by=["key", "table_index"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_yql_ab(params: ServiceParams) -> None:
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = 256 * yt_common.MB
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="yql-ab",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        yt.run_reduce(
            SqueezeYTReducer(params.squeezer, dst_indexes, key="uniqid", table_types=params.paths.types),
            source_table=params.paths.tables,
            destination_table=tmp_tables,
            reduce_by=["day", "uniqid", "testid", "slice"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_cache(params: ServiceParams) -> None:

    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = 1 * yt_common.GB
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="web from cache",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(
            sorted_exps=sorted_exps,
            dst_tables=dst_tables,
            client=params.client,
            from_cache_day=params.day if params.cache_versions else None,
        )

        tables = [
            yt.TablePath(
                path,
                lower_key=params.table_bounds.lower_reducer_key,
                upper_key=params.table_bounds.upper_reducer_key,
                client=params.client,
            )
            for path in params.paths.tables
        ]

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_REDUCE_START,
            data=dict(
                operation_sid=params.operation_sid,
                from_cache=params.cache_versions is not None,
                cache_versions=params.cache_versions.serialize(),
            )
        )

        op = yt.run_reduce(
            SqueezeYTReducer(params.squeezer, dst_indexes, key="yuid", table_types=params.paths.types),
            source_table=tables,
            destination_table=tmp_tables,
            reduce_by="yuid",
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        params.squeeze_backend.write_log(
            rectype=RectypeEnum.YT_REDUCE_FINISH,
            data=dict(
                operation_sid=params.operation_sid,
                job_statistics=op.get_job_statistics(),
                operation_id=op.id,
                operation_url=op.url,
                operation_type=op.type,
                from_cache=params.cache_versions is not None,
                cache_versions=params.cache_versions.serialize(),
            )
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)

        if params.cache_versions is not None:
            set_cache_versions(sorted_exps, tmp_tables, params.cache_versions, params.client)
        else:
            set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_intrasearch_metrika(params):
    """
    :type params: ServiceParams
    """
    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = 256 * yt_common.MB
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title="intrasearch-metrika",
                                add_acl=params.add_acl)

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        def mapper(row):
            if row["uniqid"] == "0" or not row["params"]:
                return

            parse_params = json.loads(row["params"])
            event = parse_params.pop("event", None)
            result = dict(
                yuid=row["uniqid"],
                ts=row["_logfeller_timestamp"],
                event="request" if event == "init" else event,
                event_order=0 if event == "init" else 1,
                **parse_params,
            )
            if "href" in parse_params:
                result["url"] = parse_params["href"]
            yield result

        source = yt.TablePath(
            params.paths.sources[0],
            lower_key=("1028202", "0", params.table_bounds.lower_reducer_key),
            upper_key=("1028202", "9", params.table_bounds.upper_reducer_key),
        )

        yt.run_map_reduce(
            mapper,
            SqueezeYTReducer(params.squeezer, dst_indexes, key="yuid", table_types=params.paths.types),
            source_table=source,
            destination_table=tmp_tables,
            reduce_by="yuid",
            sort_by=["yuid", "reqid", "ts", "event_order"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        sort_result_tables(tmp_tables,
                           params.client,
                           params.sort_threads,
                           yt_pool=params.yt_job_options.pool,
                           transform=params.transform)
        set_versions(sorted_exps, tmp_tables, params.versions, params.client, SqueezeWayEnum.YT)
        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


@mstand_uyt.retry_yt_operation_default
def prepare_kinopoisk(params: ServiceParams, service: str) -> None:
    assert service in {ServiceEnum.OTT_IMPRESSIONS, ServiceEnum.OTT_SESSIONS}

    sorted_exps, dst_tables, dst_indexes = find_dst_indexes(destinations=params.destinations)
    data_size_per_job = 1 * yt_common.GB
    data_size_per_job = optimize_data_size_per_job(data_size_per_job, len(sorted_exps))

    yt_spec = yt_spec_generator(squeezer=params.squeezer,
                                yt_job_options=params.yt_job_options,
                                data_size_per_job=data_size_per_job,
                                title=service,
                                add_acl=params.add_acl)

    day_str = params.day.strftime("%Y-%m-%d")

    def mapper(row: Dict[str, Any]) -> Generator[Dict[str, Any], None, None]:
        row["table_index"] = row.pop("@table_index")
        if row["table_index"] == 0:
            row["puid"] = row.pop("passportUid")
            row["experiments"] = ["{}.{}".format(key, value) for key, value in row["experiments"].items()]
            row["timestamp"] = row.pop("unixtime")

        if row["puid"] not in {None, "", "-", "None", "undefined"}:
            yield row

    next_day = params.day + datetime.timedelta(days=1)
    next_day_str = next_day.strftime("%Y-%m-%d")

    with yt.Transaction(client=params.client):
        tmp_tables = get_tmp_tables(sorted_exps=sorted_exps,
                                    dst_tables=dst_tables,
                                    client=params.client)

        if params.table_bounds.lower_reducer_key:
            daily_lower_key = (day_str, params.table_bounds.lower_reducer_key)
        else:
            daily_lower_key = day_str

        if params.table_bounds.upper_reducer_key:
            daily_upper_key = (day_str, params.table_bounds.upper_reducer_key)
        else:
            daily_upper_key = next_day_str

        sources = [
            yt.TablePath(name=params.paths.sources[0],
                         client=params.client,
                         lower_key=daily_lower_key,
                         upper_key=daily_upper_key)
        ]

        if service == ServiceEnum.OTT_IMPRESSIONS:
            sources.append(
                yt.TablePath(name=params.paths.sources[1],
                             client=params.client,
                             lower_key=daily_lower_key,
                             upper_key=daily_upper_key)
            )
        elif service == ServiceEnum.OTT_SESSIONS:
            sources.append(
                yt.TablePath(name=params.paths.sources[1],
                             client=params.client,
                             lower_key=params.table_bounds.lower_reducer_key,
                             upper_key=params.table_bounds.upper_reducer_key)
            )

        yt.run_map_reduce(
            mapper=mapper,
            reducer=SqueezeYTReducer(params.squeezer, dst_indexes, key="puid", table_types=[TableTypeEnum.SOURCE]),
            source_table=sources,
            destination_table=tmp_tables,
            reduce_by="puid",
            sort_by=["puid", "table_index", "timestamp"],
            spec=yt_spec,
            client=params.client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        errors_happened = complete_squeeze(tmp_tables, dst_tables, params.client, params.allow_empty_tables)

    if errors_happened:
        raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")


class SqueezeBackendYT:
    def __init__(
            self,
            local_files: YTBackendLocalFiles,
            config: Dict[str, Any],
            yt_job_options: YtJobOptions,
            table_bounds: TableBounds,
            yt_lock_enable: bool,
            allow_empty_tables: bool,
            sort_threads: int,
            yql_token_file: Optional[str] = None,
            add_acl: bool = True,
            enable_nile: bool = False,
            squeeze_bin_file: Optional[str] = None,
            enable_transform: bool = False,
    ) -> None:
        self.yt_job_options = yt_job_options
        self.table_bounds = table_bounds

        self._yt_lock_enable = yt_lock_enable
        self._local_files = local_files
        self._config = config
        self.allow_empty_tables = allow_empty_tables
        self.sort_threads = sort_threads
        self.add_acl = add_acl
        self.enable_nile = enable_nile
        self.squeeze_bin_file = squeeze_bin_file
        self.yql_token_file = yql_token_file
        self.transform = enable_transform

    @staticmethod
    def from_cli_args(cli_args, local_files, config) -> "SqueezeBackendYT":
        return SqueezeBackendYT(
            local_files=local_files,
            config=config,
            yt_job_options=YtJobOptions.from_cli_args(cli_args),
            table_bounds=TableBounds.from_cli_args(cli_args),
            yt_lock_enable=cli_args.yt_lock_enable,
            allow_empty_tables=cli_args.allow_empty_tables,
            sort_threads=cli_args.sort_threads,
            enable_nile=cli_args.enable_nile,
            squeeze_bin_file=cli_args.squeeze_bin_file,
            yql_token_file=cli_args.yql_token_file,
            enable_transform=cli_args.enable_transform,
        )

    def get_existing_paths(self, paths: List[str]) -> Generator[str, None, None]:
        return find_existing_tables(paths, client=self.get_client())

    def get_all_versions(self, paths: List[str]) -> Dict[str, SqueezeVersions]:
        return versions_yt.get_all_versions(paths, client=self.get_client())

    def prepare_dirs(self, paths: Set[str]) -> None:
        client = self.get_client()
        for dir_path in paths:
            yt.mkdir(dir_path, recursive=True, client=client)

    def squeeze_one_day(
            self,
            destinations: Dict[ExperimentForSqueeze, str],
            squeezer: UserSessionsSqueezer,
            paths: SqueezePaths,
            versions: SqueezeVersions,
            task: SqueezeTask,
            operation_sid: str,
            enable_binary: bool,
    ) -> Any:
        client = self.get_client()
        params = ServiceParams(
            destinations=destinations,
            squeezer=squeezer,
            paths=paths,
            local_files=self._local_files,
            versions=versions,
            client=client,
            yt_job_options=self.yt_job_options,
            table_bounds=self.table_bounds,
            transform=self.transform or task.task_count > 10,
            allow_empty_tables=self.allow_empty_tables,
            sort_threads=self.sort_threads,
            day=task.day,
            add_acl=self.add_acl,
            operation_sid=operation_sid,
            cache_versions=task.cache_versions,
            squeeze_backend=self,
            squeeze_bin_file=self.squeeze_bin_file,
        )

        if task.source == ServiceSourceEnum.TOLOKA:
            assert task.exp_dates_for_history is None
            assert not paths.yuids
            return prepare_toloka(params)

        if task.source == ServiceSourceEnum.ALICE:
            return prepare_alice(params)

        if task.source == ServiceSourceEnum.APP_METRICS:
            return prepare_app_metrics_toloka(params)

        if task.source in ServiceSourceEnum.SQUEEZE_YQL_SOURCES:
            return reduce_day_using_yql(params)

        if task.source == ServiceSourceEnum.YA_METRICS:
            return prepare_ya_metrics(params)

        if task.source == ServiceSourceEnum.TURBO:
            return prepare_turbo(params)

        if task.source == ServiceSourceEnum.ETHER:
            return prepare_ether(params)

        if task.source == ServiceSourceEnum.YA_VIDEO:
            return prepare_ya_video(params)

        if task.source == ServiceSourceEnum.ECOM:
            return prepare_ecom(params)

        if task.source == ServiceSourceEnum.PRISM:
            return prepare_prism(params)

        if task.source == ServiceSourceEnum.YQL_AB:
            return prepare_yql_ab(params)

        if task.source in ServiceSourceEnum.CACHE_SOURCES:
            return prepare_cache(params)

        if task.source == ServiceSourceEnum.INTRASEARCH_METRIKA:
            return prepare_intrasearch_metrika(params)

        if task.source == ServiceSourceEnum.OTT_IMPRESSIONS:
            return prepare_kinopoisk(params, service=ServiceEnum.OTT_IMPRESSIONS)

        if task.source == ServiceSourceEnum.OTT_SESSIONS:
            return prepare_kinopoisk(params, service=ServiceEnum.OTT_SESSIONS)

        if task.source == ServiceSourceEnum.OBJECT_ANSWER:
            return prepare_object_answer(params)

        if task.source == ServiceSourceEnum.PP:
            return prepare_pp(params)

        if task.source == ServiceSourceEnum.SURVEYS:
            return prepare_surveys(params)

        if task.source == ServiceSourceEnum.ZEN_SURVEYS:
            return prepare_zen_surveys(params)

        if task.source == ServiceSourceEnum.VH:
            return prepare_vh(params)

        if (
            task.source in ServiceSourceEnum.SQUEEZE_BIN_SOURCES
            or enable_binary and any(exp.service in ServiceEnum.SQUEEZE_BIN_SUPPORTED for exp in task.experiments)
        ):
            return reduce_day_using_bin(params)

        if self.enable_nile and task.source in ServiceSourceEnum.USE_LIBRA:
            return reduce_day_nile(params)

        with yt.Transaction(client=client):
            return reduce_day(params)

    @contextlib.contextmanager
    def lock(self, destinations: Dict[ExperimentForSqueeze, str]) -> Generator[None, None, None]:
        if not self._yt_lock_enable:
            logging.info("squeeze lock disable")
            yield
            return

        lock_paths = self._get_lock_paths(destinations)
        logging.info("Lock targets: %s", umisc.to_lines(lock_paths))

        lock_client = self.get_client()
        create_client = self.get_client()

        with yt.Transaction(client=lock_client) as transaction:
            yt_lock_wait_for = 3 * utime.Period.DAY

            for lock_path in lock_paths:
                self._create_lock_file(create_client, lock_path, 2 * yt_lock_wait_for)

                logging.info("Lock pending: %s", lock_path)
                self._lock(lock_path, yt_lock_wait_for, lock_client)
                logging.info("Lock acquired: %s", lock_path)

                acquired_process_url = unirvana.get_nirvana_workflow_url() or ""
                yt.set_attribute(lock_path, "acquired_process_url", acquired_process_url, client=create_client)
                yt.set_attribute("#{}".format(transaction.transaction_id), "acquired_process_url", acquired_process_url,
                                 client=create_client)

            try:
                yield
            finally:
                self._set_expiration_time(create_client, lock_paths, utime.Period.HOUR)

    def get_client(self) -> Optional[yt_client.Yt]:
        if self._config is None:
            return None
        return yt_client.Yt(config=self._config)

    def get_filters(self, day):
        from adminka import filter_fetcher, ab_observation

        logging.info("Start getting filters")
        filters = {}
        day_str = day.strftime("%Y%m%d")
        for i, row in enumerate(yt.read_table("//home/abt/observations", client=self.get_client())):
            if i % 10000 == 0:
                logging.info("Processed %i rows", i + 1)
            datestart = row["datestart"] or "99999999"
            dateend = row["dateend"] or "00000000"
            if datestart <= day_str <= dateend and row["filters"] and all(
                filter_fetcher.libra_supports_filter(f["name"], f["value"]) for f in row["filters"]
            ):
                prepare_filters = [(f["name"], f["value"]) for f in row["filters"]]
                filters[row["uid"]] = ab_observation.ObservationFilters(filters=prepare_filters, filter_hash=row["uid"])
        logging.info("Got %i filters", len(filters))
        return filters

    def get_cache_squeeze_info(self) -> Dict[str, Dict[Any, Any]]:
        logging.info("Get cache squeeze info")
        squeeze_info = {}
        for srv in ServiceEnum.CACHE_SUPPORTED:
            cache_dir = mstand_tables.get_cache_dir(service=srv)
            if not yt.exists(cache_dir):
                logging.warning("Cache dir '%s' for %s service does not exist", cache_dir, srv)
                continue
            try:
                squeeze_info[srv] = {
                    utime.parse_date_msk(str(table)): table
                    for table in yt.list(
                        cache_dir,
                        attributes=["cache_filters", "_mstand_squeeze_versions"],
                        client=self.get_client(),
                    )
                    if not str(table).endswith(".lock")
                }
            except yt.errors.YtHttpResponseError as exc:
                if exc.is_access_denied():
                    logging.warning("Cache dir '%s' for %s service is not available", cache_dir, srv)
                else:
                    raise
        logging.info("Got cache squeeze info: %r", {k: len(v) for k, v in squeeze_info.items()})
        return squeeze_info

    def get_cache_checker(self, min_versions: SqueezeVersions,
                          need_replace: bool) -> Callable[[datetime.date, str, Optional[str]],
                                                          Tuple[bool, Union[str, SqueezeVersions]]]:
        logging.info("Build cache checker, min_versions: %r, need_replace: %r", min_versions, need_replace)
        squeeze_info = self.get_cache_squeeze_info()

        def checker(day: datetime.date, service: str,
                    filter_hash: Optional[str]) -> Tuple[bool, Union[str, SqueezeVersions]]:
            yuid_data = squeeze_info[ServiceEnum.YUID_REQID_TESTID_FILTER].get(day)
            cache_data = squeeze_info.get(service, {}).get(day)
            if not (yuid_data and cache_data):
                return False, "Cache tables don't exist: yuid_data={}, cache_data={}".format(
                    bool(yuid_data), bool(cache_data),
                )

            if filter_hash is not None and filter_hash not in yuid_data.attributes["cache_filters"]:
                return False, "filter_hash {} doesn't found in the table attribute".format(filter_hash)

            squeezer_version = squeezer_services.SQUEEZERS[service].VERSION
            try:
                raw_cache_version = cache_data.attributes[versions_yt.MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE]
            except KeyError:
                return False, f"Not found attribute {versions_yt.MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE} at {day} for {service}"
            cache_versions = versions_yt.SqueezeVersions.deserialize(raw_cache_version, update_workflow_url=True)
            cache_version = cache_versions.service_versions[service]
            min_version = min_versions.service_versions.get(service, 0)

            if need_replace and min_version <= squeezer_version <= cache_version:
                return True, cache_versions

            if not need_replace and min_version <= cache_version:
                return True, cache_versions

            return False, "Versions is bad: min_version={}, squeeze_version={}, cache_version={}".format(
                min_version, squeezer_version, cache_version,
            )

        return checker

    def write_log(self, rectype: str, data: Optional[Dict[str, Any]] = None) -> None:
        mstand_ulog.write_yt_log(
            script=ScriptEnum.MSTAND_SQUEEZE,
            rectype=rectype,
            yt_client=self.get_client(),
            data=data,
        )

    def get_cache_tables(self, ready_tables: Set[str]) -> Dict[str, Any]:
        return {
            path: raw_yt_ops.yt_get_attribute(path, versions_yt.MSTAND_SQUEEZE_FROM_CACHE_ATTRIBUTE,
                                              yt_client=self.get_client(), default_value=False)
            for path in ready_tables
        }

    @staticmethod
    def _lock(path: str, wait_for_seconds: int, client: yt_client.Yt) -> None:
        wait_for = datetime.timedelta(seconds=wait_for_seconds)
        wait_for_log = datetime.timedelta(minutes=30)

        lock_result = yt.lock(path, mode="shared", attribute_key="id", waitable=True, client=client)
        if isinstance(lock_result, dict):
            lock_id = lock_result["lock_id"]
        else:
            lock_id = lock_result
        now = datetime.datetime.now()
        log_now = datetime.datetime.now()
        acquired = False

        while datetime.datetime.now() - now < wait_for:
            if yt.get("#{}/@state".format(lock_id), client=client) == "acquired":
                acquired = True
                break

            if datetime.datetime.now() - log_now >= wait_for_log:
                acquired_process_url = yt.get_attribute(path, "acquired_process_url", client=client)
                logging.info("Waiting for process: %s", acquired_process_url)
                log_now = datetime.datetime.now()

            time.sleep(1.0)

        if not acquired:
            raise Exception("Timed out while waiting {0} for lock path {1}".format(wait_for_seconds, path))

    @staticmethod
    def _get_expiration_time(expiration_timedelta: int) -> str:
        now = datetime.datetime.utcnow()
        delta = datetime.timedelta(seconds=expiration_timedelta)
        return (now + delta).isoformat()

    @staticmethod
    def _get_lock_paths(destinations: Dict[ExperimentForSqueeze, str]) -> List[str]:
        return sorted("{}.lock".format(path) for path in usix.itervalues(destinations))

    def _create_lock_file(self, client, lock_path, wait_for):
        expiration_time = self._get_expiration_time(expiration_timedelta=wait_for)
        try:
            yt.set_attribute(lock_path, "expiration_time", expiration_time, client=client)
            logging.info("expiration time updated: %s", lock_path)

        except yt.YtError:
            logging.debug("could not update expiration time: %s", exc_info=True)
            logging.info("create lock file: %s", lock_path)
            yt.create("file", lock_path, client=client, attributes={"expiration_time": expiration_time},
                      ignore_existing=True)

    def _set_expiration_time(self, client, lock_paths, expiration_timedelta):
        expiration_time = self._get_expiration_time(expiration_timedelta=expiration_timedelta)
        for lock_path in lock_paths:
            logging.info("update expiration time: %s", lock_path)
            try:
                yt.set_attribute(lock_path, "expiration_time", expiration_time, client=client)
            except yt.YtError:
                pass

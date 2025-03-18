import datetime
import functools
import logging
import os
import time

from typing import Any, Dict, Generator, List, Optional, Union

import mstand_utils.mstand_paths_utils as mstand_upath
import mstand_utils.mstand_tables as mstand_tables
import pytlib.yt_io_helpers as yt_io
import yaqutils.misc_helpers as umisc
import yaqutils.json_helpers as ujson
import yaqutils.file_helpers as ufile
import yaqutils.time_helpers as utime
import yt.wrapper as yt
import yt.yson as yson

from mstand_enums.mstand_general_enums import YtDownloadingFormats, YsonFormats
from mstand_enums.mstand_online_enums import ServiceSourceEnum
from mstand_utils.yt_options_struct import YtJobOptions, TableSavingParams, TableFilterBounds
from yaqtypes import JsonDict


def retry_yt_operation_default(action=None, times=2):
    if action is None:
        return functools.partial(retry_yt_operation_default, times=times)

    @functools.wraps(action)
    def wrapper(*args, **kwargs):
        return umisc.retry(action,
                           args=args,
                           kwargs=kwargs,
                           message=action.__name__,
                           times=times,
                           sleep=60 * 5,
                           exception_type=yt.YtError)

    return wrapper


def poll_operations(running, poll_delay, enable_logging):
    """
    :type running: list[tuple(int, yt.operation_commands.Operation)]
    :type poll_delay: int
    :type enable_logging: bool
    :rtype: list[tuple(int, yt.operation_commands.Operation)]
    """
    time.sleep(poll_delay)
    if enable_logging:
        logging.info("Polling operations...")
    new_running = []
    for (index, yt_operation) in running:
        state = yt_operation.get_state()
        if enable_logging or state.is_finished():
            logging.info("Operation#%s (id %s) state: %s", index, yt_operation.id, state)
            yt_operation.printer(state)
        if not state.is_finished():
            new_running.append((index, yt_operation))
        elif state.is_unsuccessfully_finished():
            raise yt.YtError("Operation in poll has finished unsuccessfully")
    if enable_logging:
        logging.info("Polling is completed")
    return new_running


# TODO: use yt.OperationTracker (or yt.OperationTrackerPool)
# It requires currently unused spec builders, which leads to necessity of rewriting significant chunk of code
def async_yt_operations_poll(yt_operation_wrapper,
                             arguments,
                             max_threads,
                             poll_delay=10,
                             logging_period=15):
    """
    :type yt_operation_wrapper:
    :type arguments: iterable
    :type max_threads: int
    :type poll_delay: int
    :type logging_period: int
    """
    running = []
    count = len(arguments)
    logging_phase = 0

    logging.info("Creating poll of %s operations with %s parallel jobs maximum", count, max_threads)
    time_start = time.time()

    for index, argument in enumerate(arguments, 1):
        logging.info("Starting operation#%s, %s total", index, count)
        running.append((index, yt_operation_wrapper(argument)))
        assert len(running) <= max_threads
        while len(running) == max_threads:
            logging_phase += 1
            running = poll_operations(running, poll_delay, logging_phase % logging_period == 0)

    while running:
        logging_phase += 1
        running = poll_operations(running, poll_delay, logging_phase % logging_period == 0)

    time_end = time.time()

    logging.info("All %s operations in poll were successfully completed in %s",
                 count,
                 datetime.timedelta(seconds=time_end - time_start))


class FilterMapper:
    def __init__(self, filter_by: str, lower_key: mstand_tables.TFilterKey, upper_key: mstand_tables.TFilterKey):
        self.filter_by = filter_by
        self.lower_key = lower_key
        self.upper_key = upper_key

    def __call__(self, row: Dict[str, Any]) -> Generator[Dict[str, Any], None, None]:
        if self.filter_by not in row:
            raise Exception("There is not column called {} in row".format(self.filter_by))
        key = row[self.filter_by]
        if key is not None and self.lower_key <= key < self.upper_key:
            yield row


def download_logs(source: str,
                  dates: utime.DateRange,
                  table_filter_bounds: TableFilterBounds,
                  table_saving_params: TableSavingParams,
                  yt_client: yt.YtClient,
                  yt_options: YtJobOptions) -> None:
    assert table_saving_params.save_to_dir or table_saving_params.save_to_yt_dir
    if source == ServiceSourceEnum.TOLOKA:
        raise Exception("{} is not supported by logs downloader.".format(ServiceSourceEnum.TOLOKA))

    for day in dates:
        tables_to_sources = mstand_upath.get_tables_by_source(day=day, path_checker=yt_client.exists)
        tables = tables_to_sources[source]

        for table in tables:
            if table.path is None or not yt_client.exists(path=table.path):
                continue

            logging.info("Processing table {}...".format(table.path))
            _filter_and_save_table(table=table,
                                   table_filter_bounds=table_filter_bounds,
                                   yt_client=yt_client,
                                   yt_options=yt_options,
                                   table_saving_params=table_saving_params)


def download_squeezes(services: List[str],
                      dates: utime.DateRange,
                      testids: List[str],
                      table_filter_bounds: TableFilterBounds,
                      table_saving_params: TableSavingParams,
                      yt_client: yt.YtClient,
                      yt_options: YtJobOptions,
                      squeeze_path: str = mstand_tables.DEFAULT_SQUEEZE_PATH,
                      filter_hash: Optional[str] = None) -> None:
    assert table_saving_params.save_to_dir or table_saving_params.save_to_yt_dir
    for service in services:
        for day in dates:
            for testid in testids:
                table = mstand_tables.MstandDailyTable(day=day,
                                                       service=service,
                                                       testid=testid,
                                                       dates=dates,
                                                       filter_hash=filter_hash,
                                                       squeeze_path=squeeze_path)
                if not yt_client.exists(path=table.path):
                    logging.warning("Skipping table {} because it doesn't exist".format(table.path))
                    continue
                logging.info("Processing table {}...".format(table.path))
                _filter_and_save_table(table=table,
                                       table_filter_bounds=table_filter_bounds,
                                       yt_client=yt_client,
                                       yt_options=yt_options,
                                       table_saving_params=table_saving_params)


def get_online_yt_operation_acl() -> List[JsonDict]:
    return [
        {
            "subjects": ["idm-group:44732"],
            "action": "allow",
            "permissions": ["read", "manage"],
        },
    ]


class UnicodeDecoder:
    def __init__(self):
        self.errors = []

    @staticmethod
    def decode(value):
        if isinstance(value, bytes):
            return value.decode("utf-8")
        return value

    @staticmethod
    def decode_value(value):
        if isinstance(value, dict):
            return {UnicodeDecoder.decode(k): UnicodeDecoder.decode_value(v) for k, v in value.items()}
        if isinstance(value, list):
            return [UnicodeDecoder.decode_value(v) for v in value]
        return UnicodeDecoder.decode(value)

    def decode_row(self, row):
        result = {}
        self.errors = []
        for k, v in row.items():
            try:
                result[self.decode(k)] = self.decode_value(v)
            except UnicodeDecodeError:
                self.errors.append("utf-8 decode error: {}: {}".format(k, v))
        return result

    @staticmethod
    def decode_rows(rows):
        for row in rows:
            yield UnicodeDecoder.decode_value(row)


def _filter_table(table: mstand_tables.BaseTable,
                  schema: yson.yson_types.YsonList,
                  table_filter_bounds: TableFilterBounds,
                  yt_options: YtJobOptions,
                  yt_client: yt.YtClient) -> Union[yt.TablePath, str]:
    lower_key, upper_key = table.get_lower_upper_filter_keys(lower_yuid=table_filter_bounds.lower_filter_key,
                                                             upper_yuid=table_filter_bounds.upper_filter_key)
    is_sorted = yt_client.is_sorted(table=table.path)

    logging.info(
        "Filtering table {} by following yuids range:"
        " ({}, {})...".format(table.path, table_filter_bounds.lower_filter_key, table_filter_bounds.upper_filter_key)
    )
    if table.filter_by is None:
        if not is_sorted:
            raise Exception("Table {} isn't sorted. Use filter_by attribute"
                            "for unsorted tables.".format(table.path))
        filtered_table = yt.TablePath(name=table.path,
                                      lower_key=lower_key,
                                      upper_key=upper_key,
                                      exact_key=table.exact_key)
    else:
        filtered_table = yt_client.create_temp_table("//tmp", "mstand_tmp_filtered_")
        logging.info("Filtering table {} on {}, saving results to"
                     "{}...".format(table.path, table.filter_by, filtered_table))

        yt_spec = yt_io.get_yt_operation_spec(yt_pool=yt_options.pool,
                                              max_failed_job_count=1,
                                              use_scheduling_tag_filter=False,
                                              tentative_job_spec=yt_options.get_tentative_job_spec())

        source_table = yt.TablePath(name=table.path, exact_key=table.exact_key, ranges=table.ranges)
        mapper = FilterMapper(filter_by=table.filter_by,
                              lower_key=lower_key,
                              upper_key=upper_key)
        yt_client.create(type="table",
                         path=filtered_table,
                         recursive=True,
                         force=True,
                         attributes={"schema": schema})
        yt_client.run_map(mapper,
                          source_table=source_table,
                          destination_table=filtered_table,
                          spec=yt_spec,
                          ordered=is_sorted)
        logging.info("Table {} was successfully filtered on {} and saved "
                     "to {}".format(table.path, table.filter_by, filtered_table))
    return filtered_table


def _create_local_table_meta(meta: yson.yson_types.YsonType, table_saving_params: TableSavingParams) -> JsonDict:
    attributes: JsonDict = {}
    if meta.get("sorted_by"):
        attributes["sorted_by"] = meta["sorted_by"]
    if meta.get("schema"):
        attributes["schema"] = meta["schema"]
    if meta.get("_mstand_squeeze_versions"):
        attributes["_mstand_squeeze_versions"] = meta["_mstand_squeeze_versions"]
    local_table_meta = {
        "attributes": attributes,
        "type": "table",
    }
    if table_saving_params.dumping_format == YtDownloadingFormats.JSON:
        encode_utf8_as_str = str(table_saving_params.encode_utf8).lower()
        local_table_meta["format"] = "<encode_utf8=%{}>json".format(encode_utf8_as_str)
    elif table_saving_params.dumping_format == YtDownloadingFormats.YSON:
        local_table_meta["format"] = "<format={}>yson".format(table_saving_params.yson_format)
    return local_table_meta


def _save_filtered_table_to_local_path(filtered_table: Union[yt.TablePath, str],
                                       dest_path: str,
                                       table_saving_params: TableSavingParams,
                                       meta: yson.yson_types.YsonType,
                                       yt_client: yt.YtClient) -> None:
    if table_saving_params.dumping_format == YtDownloadingFormats.JSON:
        yt_format = yt.JsonFormat(encode_utf8=table_saving_params.encode_utf8)
    elif table_saving_params.dumping_format == YtDownloadingFormats.YSON:
        if table_saving_params.yson_format not in YsonFormats.ALL:
            raise Exception("{} isn't supported by yson".format(table_saving_params.yson_format))
        yt_format = yt.YsonFormat(format=table_saving_params.yson_format)
    else:
        raise Exception("Unsupported format {}. Logs downloader supports "
                        "following formats: {}".format(table_saving_params.dumping_format, YtDownloadingFormats.ALL))

    logging.info("Saving filtered table to local path {}...".format(dest_path))
    result_table = yt_client.read_table(table=filtered_table, format=yt_format)

    dest_dir, _ = os.path.split(dest_path)
    ufile.make_dirs(dest_dir)
    meta_data_path = dest_path + ".meta"

    logging.info("Will dump rows into local file {} and dump meta into local file {}".format(dest_path, meta_data_path))
    is_binary = (table_saving_params.dumping_format == YtDownloadingFormats.YSON)

    with ufile.fopen_write(dest_path, use_unicode=table_saving_params.encode_utf8, is_binary=is_binary) as table_file:
        if not result_table:
            logging.warning("Table {} appears to be empty".format(dest_dir))
        if table_saving_params.dumping_format == YtDownloadingFormats.YSON:
            yson.dump(object=result_table,
                      stream=table_file,
                      yson_type="list_fragment",
                      yson_format=table_saving_params.yson_format)
        elif table_saving_params.dumping_format == YtDownloadingFormats.JSON:
            for row in result_table:
                ujson.dump_to_fd(obj=row, fd=table_file, pretty=True)
        logging.info("Dumping of table to {} has successfully completed".format(dest_path))

    local_table_meta = _create_local_table_meta(meta=meta, table_saving_params=table_saving_params)

    with ufile.fopen_write(meta_data_path, is_binary=True) as meta_file:
        yson.dump(object=local_table_meta, stream=meta_file, yson_format="pretty")
        logging.info("Meta data has been successfully dumped to {}".format(meta_data_path))

    logging.info(
        "Result table was successfully saved locally to {}".format(dest_path)
    )


def _save_filtered_table_to_yt(filtered_table: Union[yt.TablePath, str],
                               dest_yt_path: str,
                               schema: yson.yson_types.YsonList,
                               use_filter_by: bool,
                               yt_client: yt.YtClient) -> None:
    logging.info("Saving filtered table on YT to {}...".format(dest_yt_path))
    if not use_filter_by:
        yt_client.create(type="table",
                         path=dest_yt_path,
                         recursive=True,
                         force=True,
                         attributes={"schema": schema})
        result_table = yt_client.read_table(table=filtered_table)
        yt_client.write_table(table=dest_yt_path, input_stream=result_table)
    else:
        yt_client.copy(source_path=filtered_table,
                       destination_path=dest_yt_path,
                       recursive=True,
                       force=True)
    logging.info(
        "Result table was successfully saved on YT to {}".format(dest_yt_path)
    )


def _filter_and_save_table(table: mstand_tables.BaseTable,
                           table_filter_bounds: TableFilterBounds,
                           table_saving_params: TableSavingParams,
                           yt_client: yt.YtClient,
                           yt_options: YtJobOptions) -> None:
    meta = yt_client.get_attribute(path=table.path, attribute="")
    filtered_table = _filter_table(table=table,
                                   table_filter_bounds=table_filter_bounds,
                                   schema=meta["schema"],
                                   yt_options=yt_options,
                                   yt_client=yt_client)

    if table_saving_params.save_to_dir:
        full_dest_local_path = os.path.join(table_saving_params.save_to_dir, table.path.lstrip("/"))
        _save_filtered_table_to_local_path(filtered_table=filtered_table,
                                           dest_path=full_dest_local_path,
                                           table_saving_params=table_saving_params,
                                           meta=meta,
                                           yt_client=yt_client)

    if table_saving_params.save_to_yt_dir:
        full_dest_yt_path = os.path.join(table_saving_params.save_to_yt_dir, table.path.lstrip("/"))
        _save_filtered_table_to_yt(filtered_table=filtered_table,
                                   dest_yt_path=full_dest_yt_path,
                                   schema=meta["schema"],
                                   use_filter_by=table.filter_by is not None,
                                   yt_client=yt_client)
